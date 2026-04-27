/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if BOARD_ESP32_S3_LCD_2_8==1 && CONFIG_USE_LVGL == 1

#include "lvgl_log_display.h"

#include <vector>
#include <cstring>

#include "src/framework/sys/log.h"
#include "src/framework/sys/settings.h"

#define TAG "LvglLogDisplay"

LvglLogDisplay::LvglLogDisplay(DispDriver* driver, DisplayFonts fonts)
        : LvglDisplay(driver, fonts) {
            
    Log::Debug(TAG, "LvglMsgDisplay constructor.");

    // Load theme from settings
    Settings settings("display", false);
    current_theme_name_ = settings.GetString("theme", "dark").c_str();

    // Update the theme
    if (current_theme_name_ == "dark") {
        current_theme_ = DARK_THEME;
    } else if (current_theme_name_ == "light") {
        current_theme_ = LIGHT_THEME;
    }
}

LvglLogDisplay::~LvglLogDisplay() {
    statusbar_ = nullptr;

    if (message_label_ != nullptr) {
        lv_obj_del(message_label_);
    }

    if (content_ != nullptr) {
        lv_obj_del(content_);
    }

    if (container_ != nullptr) {
        lv_obj_del(container_);
    }

    if( low_battery_popup_ != nullptr ) {
        lv_obj_del(low_battery_popup_);
    }
}

void LvglLogDisplay::OnInit() {
    
    Log::Info(TAG, "Init ......");
    statusbar_ = new LvglStatusBar();
    
    SetupUI();

}

void LvglLogDisplay::Rotate(uint8_t rotation) {

}

void LvglLogDisplay::SetupUI() {
    Log::Info(TAG, "SetupUI ......");
    Log::Info(TAG, "Using %s theme", current_theme_name_.c_str());

    DisplayLockGuard lock(this);

    auto screen = lv_screen_active();
    lv_obj_set_style_text_font(screen, fonts_.text_font, 0);
    lv_obj_set_style_text_color(screen, current_theme_.text, 0);
    lv_obj_set_style_bg_color(screen, current_theme_.background, 0);

    /* Container */
    container_ = lv_obj_create(screen);
    lv_obj_set_size(container_, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_flex_flow(container_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(container_, 0, 0);
    lv_obj_set_style_border_width(container_, 0, 0);
    lv_obj_set_style_pad_row(container_, 0, 0);
    lv_obj_set_style_bg_color(container_, current_theme_.background, 0);
    lv_obj_set_style_border_color(container_, current_theme_.border, 0);

    low_battery_popup_ = lv_obj_create(screen);
    lv_obj_set_scrollbar_mode(low_battery_popup_, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(low_battery_popup_, LV_HOR_RES * 0.9, fonts_.text_font->line_height * 2);
    lv_obj_align(low_battery_popup_, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(low_battery_popup_, current_theme_.low_battery, 0);
    lv_obj_set_style_radius(low_battery_popup_, 10, 0);

    low_battery_label_ = lv_label_create(low_battery_popup_);
    lv_label_set_text(low_battery_label_, "charge");
    lv_obj_set_style_text_color(low_battery_label_, lv_color_white(), 0);
    lv_obj_center(low_battery_label_);
    lv_obj_add_flag(low_battery_popup_, LV_OBJ_FLAG_HIDDEN);

    /* 状态栏 */
    if (statusbar_!=nullptr) {
        statusbar_->SetupUI(container_, current_theme_, fonts_);
    }

    /* Content area */
    content_ = lv_obj_create(container_);
    lv_obj_set_style_radius(content_, 0, 0);
    lv_obj_set_width(content_, LV_HOR_RES);
    lv_obj_set_flex_grow(content_, 1);
    lv_obj_set_style_pad_all(content_, 4, 0);
    lv_obj_set_style_bg_color(content_, current_theme_.chat_background, 0); // Background for chat area
    lv_obj_set_style_border_color(content_, current_theme_.border, 0); // Border color for chat area

    // Enable scrolling for chat content
    lv_obj_set_scrollbar_mode(content_, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(content_, LV_DIR_VER);
    
    // Create a flex container for chat messages
    lv_obj_set_flex_flow(content_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(content_, 4, 0); // Space between messages

    // We'll create chat messages dynamically in SetChatMessage
    message_label_ = nullptr;

}

void LvglLogDisplay::SetTheme(const String& theme_name) {
    DisplayLockGuard lock(this);
    
    if (theme_name == "dark" || theme_name == "DARK") {
        current_theme_ = DARK_THEME;
    } else if (theme_name == "light" || theme_name == "LIGHT") {
        current_theme_ = LIGHT_THEME;
    } else {
        // Invalid theme name, return false
        Log::Error( TAG, "Invalid theme name: %s", theme_name.c_str());
        return;
    }
    
    // Get the active screen
    lv_obj_t* screen = lv_screen_active();
    
    // Update the screen colors
    lv_obj_set_style_bg_color(screen, current_theme_.background, 0);
    lv_obj_set_style_text_color(screen, current_theme_.text, 0);
    
    // Update container colors
    if (container_ != nullptr) {
        lv_obj_set_style_bg_color(container_, current_theme_.background, 0);
        lv_obj_set_style_border_color(container_, current_theme_.border, 0);
    }
    
    if (content_ != nullptr) {
        lv_obj_set_style_bg_color(content_, current_theme_.background, 0);
        lv_obj_set_style_border_color(content_, current_theme_.border, 0);
    }

    // Update low battery popup
    if (low_battery_popup_ != nullptr) {
        lv_obj_set_style_bg_color(low_battery_popup_, current_theme_.low_battery, 0);
    }

    // Update status bar colors
    if (statusbar_ != nullptr) {
        statusbar_->SetTheme(current_theme_);
    }
    
}

void LvglLogDisplay::SetStatus(const String& status) {
    if (statusbar_!=nullptr) {
        DisplayLockGuard lock(this);
        statusbar_->SetStatus(status);
        
        last_status_update_time_ = std::chrono::system_clock::now();
    }
}

#if CONFIG_IDF_TARGET_ESP32P4
#define  MAX_MESSAGES 50
#else
#define  MAX_MESSAGES 30
#endif

void LvglLogDisplay::SetText(const String& text) {
    SetMessage("system", text);
}

void LvglLogDisplay::SetMessage(const String& kind, const String& text) {

    if (content_ == nullptr) {
        return;
    }
    
    //避免出现空的消息框
    if(text.length() == 0) return;
    
    DisplayLockGuard lock(this);
    
    // 检查消息数量是否超过限制
    uint32_t child_count = lv_obj_get_child_cnt(content_);
    if (child_count >= MAX_MESSAGES) {
        // 删除最早的消息（第一个子对象）
        lv_obj_t* first_child = lv_obj_get_child(content_, 0);
        lv_obj_t* last_child = lv_obj_get_child(content_, child_count - 1);
        if (first_child != nullptr) {
            lv_obj_del(first_child);
        }
        // Scroll to the last message immediately
        if (last_child != nullptr) {
            lv_obj_scroll_to_view_recursive(last_child, LV_ANIM_OFF);
        }
    }
    
    // Create a message bubble
    lv_obj_t* msg_bubble = lv_obj_create(content_);
    lv_obj_set_style_radius(msg_bubble, 4, 0);
    lv_obj_set_scrollbar_mode(msg_bubble, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_border_width(msg_bubble, 1, 0);
    lv_obj_set_style_border_color(msg_bubble, current_theme_.border, 0);
    lv_obj_set_style_pad_all(msg_bubble, 4, 0);

    // 复制文本
    const char *text2 = strdup(text.c_str());

    // Create the message text
    lv_obj_t* msg_text = lv_label_create(msg_bubble);
    lv_label_set_text(msg_text, text2);
    
    // 计算文本实际宽度
    lv_coord_t text_width = lv_txt_get_width(text2, text.length(), fonts_.text_font, 0);

    // 计算气泡宽度
    lv_coord_t max_width = LV_HOR_RES * 95 / 100 - 16;  // 屏幕宽度的85%
    lv_coord_t min_width = 20;  
    lv_coord_t bubble_width;
    
    // 确保文本宽度不小于最小宽度
    if (text_width < min_width) {
        text_width = min_width;
    }

    // 如果文本宽度小于最大宽度，使用文本宽度
    if (text_width < max_width) {
        bubble_width = text_width; 
    } else {
        bubble_width = max_width;
    }
    
    // 设置消息文本的宽度
    lv_obj_set_width(msg_text, bubble_width);  // 减去padding
    lv_label_set_long_mode(msg_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(msg_text, fonts_.text_font, 0);

    // 设置气泡宽度
    lv_obj_set_width(msg_bubble, bubble_width);
    lv_obj_set_height(msg_bubble, LV_SIZE_CONTENT);

    // Set alignment and style based on message role
    if (kind == "user") {
        // User messages are right-aligned with green background
        lv_obj_set_style_bg_color(msg_bubble, current_theme_.user_bubble, 0);
        // Set text color for contrast
        lv_obj_set_style_text_color(msg_text, current_theme_.text, 0);
        
        // 设置自定义属性标记气泡类型
        lv_obj_set_user_data(msg_bubble, (void*)"user");
        
        // Set appropriate width for content
        lv_obj_set_width(msg_bubble, LV_SIZE_CONTENT);
        lv_obj_set_height(msg_bubble, LV_SIZE_CONTENT);
        
        // Don't grow
        lv_obj_set_style_flex_grow(msg_bubble, 0, 0);

    } else if (kind == "agent") {
        // agent messages are left-aligned with white background
        lv_obj_set_style_bg_color(msg_bubble, current_theme_.assistant_bubble, 0);
        // Set text color for contrast
        lv_obj_set_style_text_color(msg_text, current_theme_.text, 0);
        
        // 设置自定义属性标记气泡类型
        lv_obj_set_user_data(msg_bubble, (void*)"agent");
        
        // Set appropriate width for content
        lv_obj_set_width(msg_bubble, LV_SIZE_CONTENT);
        lv_obj_set_height(msg_bubble, LV_SIZE_CONTENT);
        
        // Don't grow
        lv_obj_set_style_flex_grow(msg_bubble, 0, 0);

    } else if (kind == "system") {
        // System messages are center-aligned with light gray background
        lv_obj_set_style_bg_color(msg_bubble, current_theme_.system_bubble, 0);
        // Set text color for contrast
        lv_obj_set_style_text_color(msg_text, current_theme_.system_text, 0);
        
        // 设置自定义属性标记气泡类型
        lv_obj_set_user_data(msg_bubble, (void*)"system");
        
        // Set appropriate width for content
        lv_obj_set_width(msg_bubble, LV_SIZE_CONTENT);
        lv_obj_set_height(msg_bubble, LV_SIZE_CONTENT);
        
        // Don't grow
        lv_obj_set_style_flex_grow(msg_bubble, 0, 0);
    }
    
    // Create a full-width container for user messages to ensure right alignment
    if (kind == "user") {
        // Create a full-width container
        lv_obj_t* container = lv_obj_create(content_);
        lv_obj_set_width(container, LV_HOR_RES);
        lv_obj_set_height(container, LV_SIZE_CONTENT);
        
        // Make container transparent and borderless
        lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(container, 0, 0);
        lv_obj_set_style_pad_all(container, 0, 0);
        
        // Move the message bubble into this container
        lv_obj_set_parent(msg_bubble, container);
        
        // Right align the bubble in the container
        lv_obj_align(msg_bubble, LV_ALIGN_RIGHT_MID, -25, 0);
        
        // Auto-scroll to this container
        lv_obj_scroll_to_view_recursive(container, LV_ANIM_OFF);

    } else if (kind == "system") {
        // 为系统消息创建全宽容器以确保居中对齐
        lv_obj_t* container = lv_obj_create(content_);
        lv_obj_set_width(container, LV_HOR_RES);
        lv_obj_set_height(container, LV_SIZE_CONTENT);
        
        // 使容器透明且无边框
        lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(container, 0, 0);
        lv_obj_set_style_pad_all(container, 0, 0);
        
        // 将消息气泡移入此容器
        lv_obj_set_parent(msg_bubble, container);
        
        // 将气泡居中对齐在容器中
        lv_obj_align(msg_bubble, LV_ALIGN_LEFT_MID, 0, 0);
        
        // 自动滚动底部
        lv_obj_scroll_to_view_recursive(container, LV_ANIM_OFF);

    } else {
        // For agent messages
        // Left align agent messages
        lv_obj_align(msg_bubble, LV_ALIGN_LEFT_MID, 0, 0);

        // Auto-scroll to the message bubble
        lv_obj_scroll_to_view_recursive(msg_bubble, LV_ANIM_OFF);
    }

    // Store reference to the latest message label
    message_label_ = msg_text;

}

#endif // CONFIG_USE_LVGL
