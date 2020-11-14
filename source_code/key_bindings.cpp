mapping_init(tctx, &framework_mapping);
#if OS_MAC
setup_mac_mapping(&framework_mapping, mapid_global, mapid_file, mapid_code);
#else
MappingScope();
SelectMapping(&framework_mapping);
SelectMap(mapid_global);


BindCore(krz_startup, CoreCode_Startup);
BindCore(default_try_exit, CoreCode_TryExit);
BindCore(clipboard_record_clip, CoreCode_NewClipboardContents);

// MACRO
Bind(keyboard_macro_start_recording , KeyCode_R, KeyCode_Shift, KeyCode_Control);
Bind(keyboard_macro_finish_recording, KeyCode_Escape);
Bind(keyboard_macro_replay,           KeyCode_R, KeyCode_Control);

// OPEN LISTERS
Bind(interactive_open_or_new,       KeyCode_O, KeyCode_Control);
Bind(open_in_other,                 KeyCode_O, KeyCode_Alt);
Bind(interactive_switch_buffer,     KeyCode_P, KeyCode_Control);
Bind(command_lister,                KeyCode_P, KeyCode_Shift, KeyCode_Control);


// BUILD PANEL
Bind(build_in_build_panel,          KeyCode_F9);
Bind(change_to_build_panel,         KeyCode_F9, KeyCode_Control);

Bind(change_to_build_panel,         KeyCode_3, KeyCode_Control);
Bind(close_build_panel,             KeyCode_Escape);

// CLI
Bind(execute_any_cli,               KeyCode_F10);
Bind(execute_previous_cli,          KeyCode_F10, KeyCode_Control);


// JUMP
Bind(jump_to_last_point,            KeyCode_Q, KeyCode_Alt);
Bind(jump_to_last_point,            KeyCode_A, KeyCode_Alt);
// >???
Bind(goto_first_jump,               KeyCode_M, KeyCode_Alt, KeyCode_Shift);
Bind(goto_next_jump,                KeyCode_N, KeyCode_Alt);
Bind(goto_prev_jump,                KeyCode_N, KeyCode_Alt, KeyCode_Shift);



Bind(project_go_to_root_directory,  KeyCode_H, KeyCode_Control);
Bind(save_all_dirty_buffers,        KeyCode_S, KeyCode_Control, KeyCode_Shift);

// QUICK SWAPPING
Bind(quick_swap_buffer,             KeyCode_Tab, KeyCode_Control);
Bind(change_active_panel_backwards, KeyCode_1, KeyCode_Control);
Bind(change_active_panel,           KeyCode_2, KeyCode_Control);

// TEXT MODES 
Bind(case_sensitive_mode_toggle, KeyCode_C, KeyCode_Alt);


Bind(exit_4coder,          KeyCode_F4, KeyCode_Alt);

// Bind(krz_project_fkey_command, KeyCode_F5);
// Bind(krz_project_fkey_command, KeyCode_F6);
// Bind(krz_project_fkey_command, KeyCode_F7);
// Bind(krz_project_fkey_command, KeyCode_F8);
Bind(project_fkey_command, KeyCode_F1);
Bind(project_fkey_command, KeyCode_F2);
Bind(project_fkey_command, KeyCode_F3);
Bind(project_fkey_command, KeyCode_F4);

Bind(TODOs_list, KeyCode_F1, KeyCode_Control);
Bind(painter_mode_switch, KeyCode_F5);
Bind(painter_mode_clear, KeyCode_F6);
Bind(painter_mode_brush_size_lower, KeyCode_F7);
Bind(painter_mode_brush_size_upper, KeyCode_F8);
Bind(split_fullscreen_mode, KeyCode_F12);

Bind(load_theme_current_buffer, KeyCode_T, KeyCode_Control, KeyCode_Shift, KeyCode_Alt);

BindMouseWheel(mouse_wheel_scroll);
BindMouseWheel(mouse_wheel_change_face_size, KeyCode_Control);

// -------------------------------------------------------------- //
SelectMap(mapid_file);
ParentMap(mapid_global);

BindTextInput(write_text_input);

// MOUSE
BindMouse(krz_click_set_cursor_and_mark, MouseCode_Left);
BindMouseRelease(click_set_cursor, MouseCode_Left);
BindCore(krz_click_set_cursor_and_mark, CoreCode_ClickActivateView);
BindMouseMove(krz_click_set_cursor_if_lbutton);


Bind(put_new_line_bellow, KeyCode_Return, KeyCode_Control);
Bind(duplicate_multiple_lines_down,              KeyCode_Down, KeyCode_Shift, KeyCode_Alt);
Bind(duplicate_multiple_lines_up,              KeyCode_Up, KeyCode_Shift, KeyCode_Alt);

{
    Bind(delete_char,            KeyCode_Delete);
    Bind(backspace_char,         KeyCode_Backspace);
    Bind(delete_selected_lines,  KeyCode_K, KeyCode_Control);
    
    // MOVE
    Bind(move_up,                KeyCode_Up);
    Bind(move_down,              KeyCode_Down);
    Bind(move_left,              KeyCode_Left);
    Bind(move_right,             KeyCode_Right);
    Bind(seek_end_of_line,       KeyCode_End);
    Bind(seek_beginning_of_line, KeyCode_Home);
    Bind(page_up,                KeyCode_PageUp);
    Bind(page_down,              KeyCode_PageDown);
    Bind(goto_beginning_of_file, KeyCode_PageUp, KeyCode_Control);
    Bind(goto_end_of_file,       KeyCode_PageDown, KeyCode_Control);
    Bind(move_up_to_blank_line_end,        KeyCode_Up, KeyCode_Control);
    Bind(move_down_to_blank_line_end,      KeyCode_Down, KeyCode_Control);
    Bind(move_lines_up,                     KeyCode_Up, KeyCode_Alt);
    Bind(move_lines_down,                   KeyCode_Down, KeyCode_Alt);
    Bind(goto_line,                   KeyCode_G, KeyCode_Control);
    
    
    Bind(undo,                        KeyCode_Z, KeyCode_Control);
    Bind(redo,                        KeyCode_Z, KeyCode_Shift, KeyCode_Control);
    Bind(copy,                        KeyCode_C, KeyCode_Control);
    Bind(cut,                         KeyCode_X, KeyCode_Control);
    Bind(save,                        KeyCode_S, KeyCode_Control);
    Bind(save_all_dirty_buffers,      KeyCode_S, KeyCode_Control, KeyCode_Shift);
    
    Bind(snipe_backward_whitespace_or_token_boundary, KeyCode_Backspace, KeyCode_Control);
    Bind(snipe_forward_whitespace_or_token_boundary,  KeyCode_Delete, KeyCode_Control);
    
    Bind(krz_move_left_whitespace_boundary_or_line_boundary,    KeyCode_Left, KeyCode_Alt);
    Bind(krz_move_right_whitespace_boundary_or_line_boundary,   KeyCode_Right, KeyCode_Alt);
    Bind(krz_move_left_alpha_numeric_boundary,  KeyCode_Left, KeyCode_Control);
    Bind(krz_move_right_alpha_numeric_boundary, KeyCode_Right, KeyCode_Control);
    
#if FCODER_MODE_ORIGINAL
    Bind(set_mark, KeyCode_Shift, KeyCode_Control);
    Bind(cursor_mark_swap, KeyCode_M, KeyCode_Control);
    Bind(delete_range, KeyCode_Backspace, KeyCode_Control);
#endif
}

// SEARCH / REPLACE
Bind(search_selection_or_identifier,           KeyCode_F, KeyCode_Control);
Bind(search,                           KeyCode_F, KeyCode_Shift, KeyCode_Control);

Bind(list_all_locations_of_identifier_or_selection, KeyCode_F, KeyCode_Alt);
Bind(list_all_locations, KeyCode_F, KeyCode_Alt, KeyCode_Shift);

Bind(query_replace_selection_or_identifier,  KeyCode_D, KeyCode_Control);
Bind(krz_replace_in_all_buffers,  KeyCode_D, KeyCode_Alt);
Bind(center_view,  KeyCode_4, KeyCode_Control);

Bind(list_all_functions_all_buffers_lister, KeyCode_E, KeyCode_Alt);
Bind(list_all_functions_current_buffer_lister, KeyCode_E, KeyCode_Control);


Bind(paste_and_indent,            KeyCode_V, KeyCode_Control);
Bind(paste_next_and_indent,       KeyCode_V, KeyCode_Control, KeyCode_Shift);
Bind(if_read_only_goto_position,  KeyCode_Return);
Bind(if_read_only_goto_position_same_panel, KeyCode_Return, KeyCode_Shift);

{
    Bind(kill_buffer,                 KeyCode_W, KeyCode_Control);
    Bind(close_panel,                 KeyCode_W, KeyCode_Control, KeyCode_Shift);
    Bind(reopen,                      KeyCode_O, KeyCode_Control, KeyCode_Shift);
}
// CODE GEN / ALTERATION
{
    Bind(to_uppercase, KeyCode_U, KeyCode_Control, KeyCode_Shift);
    Bind(to_lowercase, KeyCode_L, KeyCode_Control, KeyCode_Shift);
    
    Bind(write_warning, KeyCode_W, KeyCode_Alt);
    Bind(write_todo,             KeyCode_T, KeyCode_Alt);
    Bind(write_note,                 KeyCode_N, KeyCode_Alt);
    Bind(open_long_braces,           KeyCode_LeftBracket, KeyCode_Control);
    Bind(open_long_braces_semicolon, KeyCode_LeftBracket, KeyCode_Control, KeyCode_Shift);
    Bind(open_long_braces_break,     KeyCode_RightBracket, KeyCode_Control, KeyCode_Shift);
}

// WORD COMPLETE
Bind(word_complete,              KeyCode_Tab);
Bind(word_complete_drop_down,    KeyCode_Space, KeyCode_Control);
Bind(krz_snippet_lister,         KeyCode_Tick, KeyCode_Control);

// -------------------------------------------------------------- //

SelectMap(mapid_code);
ParentMap(mapid_file);
BindTextInput(write_text_and_auto_indent);




// NOT USED MUCH
Bind(select_surrounding_scope,   KeyCode_LeftBracket, KeyCode_Alt);
Bind(select_surrounding_scope_maximal, KeyCode_LeftBracket, KeyCode_Alt, KeyCode_Shift);
Bind(select_prev_scope_absolute, KeyCode_RightBracket, KeyCode_Alt);
Bind(select_prev_top_most_scope, KeyCode_RightBracket, KeyCode_Alt, KeyCode_Shift);
Bind(select_next_scope_absolute, KeyCode_Quote, KeyCode_Alt);
Bind(select_next_scope_after_current, KeyCode_Quote, KeyCode_Alt, KeyCode_Shift);
Bind(place_in_scope,             KeyCode_ForwardSlash, KeyCode_Alt);
Bind(delete_current_scope,       KeyCode_Minus, KeyCode_Alt);
Bind(if0_off,                    KeyCode_I, KeyCode_Alt);
// Bind(open_matching_file_cpp,     KeyCode_2, KeyCode_Alt);
Bind(write_zero_struct,          KeyCode_0, KeyCode_Control);

// JUMP TO DEFINITION
Bind(open_file_in_quotes,          KeyCode_A, KeyCode_Control);
Bind(jump_to_definition_at_cursor, KeyCode_Q, KeyCode_Control);

// COMMENTS
Bind(krz_comment_lines_toggle,        KeyCode_ForwardSlash, KeyCode_Control);
Bind(write_block,                KeyCode_ForwardSlash, KeyCode_Control, KeyCode_Shift);
#endif