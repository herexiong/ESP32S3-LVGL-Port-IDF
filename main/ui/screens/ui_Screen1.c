// This file was generated by SquareLine Studio
// SquareLine Studio version: SquareLine Studio 1.4.1
// LVGL version: 8.3.6
// Project name: MyFirst_SquareLine_Project

#include "../ui.h"

void ui_Screen1_screen_init(void)
{
    ui_Screen1 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen1, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    ui_Panel1 = lv_obj_create(ui_Screen1);
    lv_obj_set_width(ui_Panel1, 148);
    lv_obj_set_height(ui_Panel1, 200);
    lv_obj_set_x(ui_Panel1, -79);
    lv_obj_set_y(ui_Panel1, -130);
    lv_obj_set_align(ui_Panel1, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_Panel1, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    ui_Arc1 = lv_arc_create(ui_Panel1);
    lv_obj_set_width(ui_Arc1, 109);
    lv_obj_set_height(ui_Arc1, 106);
    lv_obj_set_x(ui_Arc1, 1);
    lv_obj_set_y(ui_Arc1, -8);
    lv_obj_set_align(ui_Arc1, LV_ALIGN_CENTER);
    lv_arc_set_value(ui_Arc1, 0);


    ui_Label1 = lv_label_create(ui_Panel1);
    lv_obj_set_width(ui_Label1, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label1, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Label1, 1);
    lv_obj_set_y(ui_Label1, 50);
    lv_obj_set_align(ui_Label1, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label1, "0%");

    ui_TextArea1 = lv_textarea_create(ui_Screen1);
    lv_obj_set_width(ui_TextArea1, 150);
    lv_obj_set_height(ui_TextArea1, 70);
    lv_obj_set_x(ui_TextArea1, 77);
    lv_obj_set_y(ui_TextArea1, -196);
    lv_obj_set_align(ui_TextArea1, LV_ALIGN_CENTER);
    lv_textarea_set_placeholder_text(ui_TextArea1, "Input anything...");



    ui_TextArea2 = lv_textarea_create(ui_Screen1);
    lv_obj_set_width(ui_TextArea2, 150);
    lv_obj_set_height(ui_TextArea2, 70);
    lv_obj_set_x(ui_TextArea2, 76);
    lv_obj_set_y(ui_TextArea2, -114);
    lv_obj_set_align(ui_TextArea2, LV_ALIGN_CENTER);
    lv_textarea_set_placeholder_text(ui_TextArea2, "Placeholder...");



    ui_Keyboard2 = lv_keyboard_create(ui_Screen1);
    lv_obj_set_width(ui_Keyboard2, 320);
    lv_obj_set_height(ui_Keyboard2, 170);
    lv_obj_set_x(ui_Keyboard2, 0);
    lv_obj_set_y(ui_Keyboard2, 326);
    lv_obj_set_align(ui_Keyboard2, LV_ALIGN_CENTER);

    ui_Panel2 = lv_obj_create(ui_Screen1);
    lv_obj_set_width(ui_Panel2, 148);
    lv_obj_set_height(ui_Panel2, 50);
    lv_obj_set_x(ui_Panel2, -78);
    lv_obj_set_y(ui_Panel2, 7);
    lv_obj_set_align(ui_Panel2, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_Panel2, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    ui_Label2 = lv_label_create(ui_Panel2);
    lv_obj_set_width(ui_Label2, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label2, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Label2, 42);
    lv_obj_set_y(ui_Label2, 0);
    lv_obj_set_align(ui_Label2, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label2, "OFF");

    ui_Switch1 = lv_switch_create(ui_Panel2);
    lv_obj_set_width(ui_Switch1, 50);
    lv_obj_set_height(ui_Switch1, 25);
    lv_obj_set_x(ui_Switch1, -40);
    lv_obj_set_y(ui_Switch1, -1);
    lv_obj_set_align(ui_Switch1, LV_ALIGN_CENTER);


    lv_obj_add_event_cb(ui_Arc1, ui_event_Arc1, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_TextArea1, ui_event_TextArea1, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_TextArea2, ui_event_TextArea2, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_Keyboard2, ui_event_Keyboard2, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_Switch1, ui_event_Switch1, LV_EVENT_ALL, NULL);

}
