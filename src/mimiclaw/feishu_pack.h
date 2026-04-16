#ifndef _FEISHU_PACK_H
#define _FEISHU_PACK_H

#include <Arduino.h>

/* ── Feishu WS frame (protobuf) ────────────────────────────── */
typedef struct {
    char key[32];               //      4 byte
    char value[128];            //     16 byte
} ws_header_t;


typedef struct {
    uint64_t seq_id;            //      8 byte
    uint64_t log_id;            //      8 byte
    int32_t service;            //      4 byte
    int32_t method;             //      4 byte
    ws_header_t headers[16];    //    320 byte
    size_t header_count;        //      4 byte
    const uint8_t *payload;     //       
    size_t payload_len;         //      4 byte
} ws_frame_t;


static bool pb_read_varint(const uint8_t *buf, size_t len, size_t *pos, uint64_t *out)
{
    uint64_t v = 0;
    int shift = 0;
    while (*pos < len && shift <= 63) {
        uint8_t b = buf[(*pos)++];
        v |= ((uint64_t)(b & 0x7F)) << shift;
        if ((b & 0x80) == 0) {
            *out = v;
            return true;
        }
        shift += 7;
    }
    return false;
}

static bool pb_skip_field(const uint8_t *buf, size_t len, size_t *pos, uint8_t wire_type)
{
    uint64_t n = 0;
    switch (wire_type) {
        case 0:
            return pb_read_varint(buf, len, pos, &n);
        case 1:
            if (*pos + 8 > len) return false;
            *pos += 8;
            return true;
        case 2:
            if (!pb_read_varint(buf, len, pos, &n)) return false;
            if (*pos + (size_t)n > len) return false;
            *pos += (size_t)n;
            return true;
        case 5:
            if (*pos + 4 > len) return false;
            *pos += 4;
            return true;
        default:
            return false;
    }
}

static bool pb_parse_header_msg(const uint8_t *buf, size_t len, ws_header_t *h)
{
    memset(h, 0, sizeof(*h));
    size_t pos = 0;
    while (pos < len) {
        uint64_t tag = 0, slen = 0;
        if (!pb_read_varint(buf, len, &pos, &tag)) return false;
        uint32_t field = (uint32_t)(tag >> 3);
        uint8_t wt = (uint8_t)(tag & 0x07);
        if (wt != 2) {
            if (!pb_skip_field(buf, len, &pos, wt)) return false;
            continue;
        }
        if (!pb_read_varint(buf, len, &pos, &slen)) return false;
        if (pos + (size_t)slen > len) return false;
        if (field == 1) {
            size_t n = (slen < sizeof(h->key) - 1) ? (size_t)slen : sizeof(h->key) - 1;
            memcpy(h->key, buf + pos, n);
            h->key[n] = '\0';
        } else if (field == 2) {
            size_t n = (slen < sizeof(h->value) - 1) ? (size_t)slen : sizeof(h->value) - 1;
            memcpy(h->value, buf + pos, n);
            h->value[n] = '\0';
        }
        pos += (size_t)slen;
    }
    return true;
}

static bool pb_parse_frame(const uint8_t *buf, size_t len, ws_frame_t *f)
{
    memset(f, 0, sizeof(*f));
    size_t pos = 0;
    while (pos < len) {
        uint64_t tag = 0, v = 0, blen = 0;
        if (!pb_read_varint(buf, len, &pos, &tag)) return false;
        uint32_t field = (uint32_t)(tag >> 3);
        uint8_t wt = (uint8_t)(tag & 0x07);
        if (field == 1 && wt == 0) {
            if (!pb_read_varint(buf, len, &pos, &f->seq_id)) return false;
        } else if (field == 2 && wt == 0) {
            if (!pb_read_varint(buf, len, &pos, &f->log_id)) return false;
        } else if (field == 3 && wt == 0) {
            if (!pb_read_varint(buf, len, &pos, &v)) return false;
            f->service = (int32_t)v;
        } else if (field == 4 && wt == 0) {
            if (!pb_read_varint(buf, len, &pos, &v)) return false;
            f->method = (int32_t)v;
        } else if (field == 5 && wt == 2) {
            if (!pb_read_varint(buf, len, &pos, &blen)) return false;
            if (pos + (size_t)blen > len) return false;
            if (f->header_count < 16) {
                pb_parse_header_msg(buf + pos, (size_t)blen, &f->headers[f->header_count++]);
            }
            pos += (size_t)blen;
        } else if (field == 8 && wt == 2) {
            if (!pb_read_varint(buf, len, &pos, &blen)) return false;
            if (pos + (size_t)blen > len) return false;
            f->payload = buf + pos;
            f->payload_len = (size_t)blen;
            pos += (size_t)blen;
        } else {
            if (!pb_skip_field(buf, len, &pos, wt)) return false;
        }
    }
    return true;
}

static const char *frame_header_value(const ws_frame_t *f, const char *key)
{
    for (size_t i = 0; i < f->header_count; i++) {
        if (strcmp(f->headers[i].key, key) == 0) {
            return f->headers[i].value;
        }
    }
    return NULL;
}

static bool pb_write_varint(uint8_t *buf, size_t cap, size_t *pos, uint64_t value)
{
    do {
        if (*pos >= cap) return false;
        uint8_t byte = (uint8_t)(value & 0x7F);
        value >>= 7;
        if (value) byte |= 0x80;
        buf[(*pos)++] = byte;
    } while (value);
    return true;
}

static bool pb_write_tag(uint8_t *buf, size_t cap, size_t *pos, uint32_t field, uint8_t wt)
{
    return pb_write_varint(buf, cap, pos, ((uint64_t)field << 3) | wt);
}

static bool pb_write_bytes(uint8_t *buf, size_t cap, size_t *pos, uint32_t field, const uint8_t *data, size_t len)
{
    if (!pb_write_tag(buf, cap, pos, field, 2)) return false;
    if (!pb_write_varint(buf, cap, pos, len)) return false;
    if (*pos + len > cap) return false;
    memcpy(buf + *pos, data, len);
    *pos += len;
    return true;
}

static bool pb_write_string(uint8_t *buf, size_t cap, size_t *pos, uint32_t field, const char *s)
{
    return pb_write_bytes(buf, cap, pos, field, (const uint8_t *)s, strlen(s));
}

static bool ws_encode_header(uint8_t *dst, size_t cap, size_t *out_len, const char *key, const char *value)
{
    size_t pos = 0;
    if (!pb_write_string(dst, cap, &pos, 1, key)) return false;
    if (!pb_write_string(dst, cap, &pos, 2, value)) return false;
    *out_len = pos;
    return true;
}

#endif //_FEISHU_PACK_H