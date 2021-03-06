/*
4coder_default_bidings.cpp - Supplies the default bindings used for default 4coder behavior.
*/

/***********************************************************************************
*
*   TODO:
*   QueryReplace in all buffers where it asks you about every replace
*   Fix move lines at the edge of buffer
*   find all @ find all TODOs etc.
*   Close lister with control W
*
************************************************************************************/

#define FCODER_MODE_ORIGINAL 0
#define BIG_CURSOR 1
#define HIGHLIGH_SELECTION_MATCH 1
#define BAR_POSITION_BOT 0 // 0 for top

#if !defined(FCODER_DEFAULT_BINDINGS_CPP)
#define FCODER_DEFAULT_BINDINGS_CPP

#include "4coder_default_include.cpp"

global bool case_sensitive_mode = false;
global bool is_fullscreen_mode;
global f32 margin_width = 1.f;
global i32 max_selection_size = 128;

CUSTOM_ID( command_map, mapid_krz_mode );
CUSTOM_ID( colors, selection_match_highlight );
CUSTOM_ID( colors, primitive_highlight_type );
CUSTOM_ID( colors, primitive_highlight_function );
CUSTOM_ID( colors, primitive_highlight_macro );
CUSTOM_ID( colors, primitive_highlight_4coder_command );
CUSTOM_ID( colors, vertical_scope_annotation_text );
CUSTOM_ID( colors, vertical_scope_annotation_highlight );
CUSTOM_ID( colors, compilation_buffer_color );

#if !defined(META_PASS)
#include "generated/managed_id_metadata.cpp"
#endif

#include "primitive_highlight.cpp"
#include "vertical_scope_annotations.cpp"
#include "painter_mode.cpp"
#include "selection_based_cursor_improvements.cpp"
#include "snippets.cpp"
#include "todo.cpp"
#include "search.cpp"

static void
F4_RenderErrorAnnotations(Application_Links *app, Buffer_ID buffer,
                          Text_Layout_ID text_layout_id,
                          Buffer_ID jump_buffer)
{
    ProfileScope(app, "[Fleury] Error Annotations");
    
    Heap *heap = &global_heap;
    Scratch_Block scratch(app);
    
    Locked_Jump_State jump_state = {};
    {
        ProfileScope(app, "[Fleury] Error Annotations (Get Locked Jump State)");
        jump_state = get_locked_jump_state(app, heap);
    }
    
    Face_ID face = get_face_id(app, buffer);
    Face_Metrics metrics = get_face_metrics(app, face);
    
    if(jump_buffer != 0 && jump_state.view != 0)
    {
        Managed_Scope buffer_scopes[2];
        {
            ProfileScope(app, "[Fleury] Error Annotations (Buffer Get Managed Scope)");
            buffer_scopes[0] = buffer_get_managed_scope(app, jump_buffer);
            buffer_scopes[1] = buffer_get_managed_scope(app, buffer);
        }
        
        Managed_Scope comp_scope = 0;
        {
            ProfileScope(app, "[Fleury] Error Annotations (Get Managed Scope)");
            comp_scope = get_managed_scope_with_multiple_dependencies(app, buffer_scopes, ArrayCount(buffer_scopes));
        }
        
        Managed_Object *buffer_markers_object = 0;
        {
            ProfileScope(app, "[Fleury] Error Annotations (Scope Attachment)");
            buffer_markers_object = scope_attachment(app, comp_scope, sticky_jump_marker_handle, Managed_Object);
        }
        
        // NOTE(rjf): Get buffer markers (locations where jumps point at).
        i32 buffer_marker_count = 0;
        Marker *buffer_markers = 0;
        {
            ProfileScope(app, "[Fleury] Error Annotations (Load Managed Object Data)");
            buffer_marker_count = managed_object_get_item_count(app, *buffer_markers_object);
            buffer_markers = push_array(scratch, Marker, buffer_marker_count);
            managed_object_load_data(app, *buffer_markers_object, 0, buffer_marker_count, buffer_markers);
        }
        
        i64 last_line = -1;
        
        for(i32 i = 0; i < buffer_marker_count; i += 1)
        {
            ProfileScope(app, "[Fleury] Error Annotations (Buffer Loop)");
            
            i64 jump_line_number = get_line_from_list(app, jump_state.list, i);
            i64 code_line_number = get_line_number_from_pos(app, buffer, buffer_markers[i].pos);
            
            if(code_line_number != last_line)
            {
                ProfileScope(app, "[Fleury] Error Annotations (Jump Line)");
                
                String_Const_u8 jump_line = push_buffer_line(app, scratch, jump_buffer, jump_line_number);
                
                // NOTE(rjf): Remove file part of jump line.
                {
                    u64 index = string_find_first(jump_line, string_u8_litexpr("error"), StringMatch_CaseInsensitive);
                    if(index == jump_line.size)
                    {
                        index = string_find_first(jump_line, string_u8_litexpr("warning"), StringMatch_CaseInsensitive);
                        if(index == jump_line.size)
                        {
                            index = 0;
                        }
                    }
                    jump_line.str += index;
                    jump_line.size -= index;
                }
                
                // NOTE(rjf): Render annotation.
                {
                    Range_i64 line_range = Ii64(code_line_number);
                    Range_f32 y1 = text_layout_line_on_screen(app, text_layout_id, line_range.min);
                    Range_f32 y2 = text_layout_line_on_screen(app, text_layout_id, line_range.max);
                    Range_f32 y = range_union(y1, y2);
                    Rect_f32 last_character_on_line_rect =
                        text_layout_character_on_screen(app, text_layout_id, get_line_end_pos(app, buffer, code_line_number)-1);
                    
                    if(range_size(y) > 0.f)
                    {
                        Rect_f32 region = text_layout_region(app, text_layout_id);
                        Vec2_f32 draw_position =
                        {
                            region.x1 - metrics.max_advance*jump_line.size -
                                (y.max-y.min)/2 - metrics.line_height/2,
                            y.min + (y.max-y.min)/2 - metrics.line_height/2,
                        };
                        
                        if(draw_position.x < last_character_on_line_rect.x1 + 30)
                        {
                            draw_position.x = last_character_on_line_rect.x1 + 30;
                        }
                        
                        draw_string(app, face, jump_line, draw_position, 0xffff0000);
                        
                        // Mouse_State mouse_state = get_mouse_state(app);
                        // if(mouse_state.x >= region.x0 && mouse_state.x <= region.x1 &&
                        //    mouse_state.y >= y.min && mouse_state.y <= y.max)
                        // {
                        //     F4_PushTooltip(jump_line, 0xffff0000);
                        // }
                    }
                }
            }
            
            last_line = code_line_number;
        }
    }
}

function void
krz_render_buffer(Application_Links *app, View_ID view_id, Face_ID face_id,
                  Buffer_ID buffer, Text_Layout_ID text_layout_id,
                  Rect_f32 rect){
    ProfileScope(app, "render buffer");
    
    View_ID active_view = get_active_view(app, Access_Always);
    b32 is_active_view = (active_view == view_id);
    Rect_f32 prev_clip = draw_set_clip(app, rect);
    
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    
    // NOTE(allen): Cursor shape
    Face_Metrics metrics = get_face_metrics(app, face_id);
    f32 cursor_roundness = metrics.normal_advance*global_config.cursor_roundness;
    f32 mark_thickness = (f32)global_config.mark_thickness;
    
    // NOTE(rjf): Error annotations
    {
        String_Const_u8 name = string_u8_litexpr("*compilation*");
        Buffer_ID compilation_buffer = get_buffer_by_name(app, name, Access_Always);
        F4_RenderErrorAnnotations(app, buffer, text_layout_id, compilation_buffer);
    }
    
    // NOTE(allen): Token colorizing
    Token_Array token_array = get_token_array_from_buffer(app, buffer);
    if (token_array.tokens != 0){
        primitive_highlight_draw_cpp_token_colors( app, text_layout_id, &token_array, buffer );
        
        // NOTE(allen): Scan for TODOs and NOTEs
        if (global_config.use_comment_keyword){
            Comment_Highlight_Pair pairs[] = {
                {string_u8_litexpr("NOTE"), finalize_color(defcolor_comment_pop, 0)},
                {string_u8_litexpr("TODO"), finalize_color(defcolor_comment_pop, 1)},
                {string_u8_litexpr("WARNING"), finalize_color(defcolor_comment_pop, 1)},
                {string_u8_litexpr("FIXME"), finalize_color(defcolor_comment_pop, 1)},
            };
            draw_comment_highlights(app, buffer, text_layout_id,
                                    &token_array, pairs, ArrayCount(pairs));
        }
    }
    else{
        paint_text_color_fcolor(app, text_layout_id, visible_range, fcolor_id(defcolor_text_default));
    }
    
    i64 cursor_pos = view_correct_cursor(app, view_id);
    i64 mark_pos = view_correct_mark(app, view_id);
    
#if HIGHLIGH_SELECTION_MATCH
    Scratch_Block scratch(app);
    
    Range_i64 selection = {
        Min(cursor_pos, mark_pos),
        Max(cursor_pos, mark_pos)
    };
    
    i64 line_min = get_line_number_from_pos(app, buffer, selection.min);
    i64 line_max = get_line_number_from_pos(app, buffer, selection.max);
    
    if(line_min == line_max)
    {
        
        String_Const_u8 selected_string = push_buffer_range(app, scratch, 
                                                            buffer, selection);
        
        local_persist Character_Predicate *pred =
            &character_predicate_alpha_numeric_underscore_utf8;
        
        String_Match_List matches = buffer_find_all_matches(app, scratch, buffer,  0, 
                                                            visible_range, 
                                                            selected_string, pred, 
                                                            Scan_Forward);
        String_Match_Flag must_have_flags = StringMatch_CaseSensitive;
        string_match_list_filter_flags(&matches, must_have_flags, 0);
        
        for (String_Match *node = matches.first; node != 0; node = node->next){
            
            draw_character_block(app, text_layout_id, 
                                 node->range, 0, 
                                 finalize_color(selection_match_highlight, 0));
        }
    }
#endif
    
    
    // NOTE(allen): Scope highlight
    if (global_config.use_scope_highlight){
        Color_Array colors = finalize_color_array(defcolor_back_cycle);
        draw_scope_highlight(app, buffer, text_layout_id, cursor_pos, colors.vals, colors.count);
    }
    
    if (global_config.use_error_highlight || global_config.use_jump_highlight){
        // NOTE(allen): Error highlight
        String_Const_u8 name = string_u8_litexpr("*compilation*");
        Buffer_ID compilation_buffer = get_buffer_by_name(app, name, Access_Always);
        if (global_config.use_error_highlight){
            draw_jump_highlights(app, buffer, text_layout_id, compilation_buffer,
                                 fcolor_id(defcolor_highlight_junk));
        }
        
        // NOTE(allen): Search highlight
        if (global_config.use_jump_highlight){
            Buffer_ID jump_buffer = get_locked_jump_buffer(app);
            if (jump_buffer != compilation_buffer){
                draw_jump_highlights(app, buffer, text_layout_id, jump_buffer,
                                     fcolor_id(defcolor_highlight_white));
            }
        }
    }
    
    // NOTE(allen): Color parens
    if (global_config.use_paren_helper){
        Color_Array colors = finalize_color_array(defcolor_text_cycle);
        draw_paren_highlight(app, buffer, text_layout_id, cursor_pos, colors.vals, colors.count);
    }
    
    // NOTE(allen): Line highlight
    if (global_config.highlight_line_at_cursor && is_active_view){
        i64 line_number = get_line_number_from_pos(app, buffer, cursor_pos);
        draw_line_highlight(app, text_layout_id, line_number,
                            fcolor_id(defcolor_highlight_cursor_line));
    }
    
    // NOTE(allen): Whitespace highlight
    b64 show_whitespace = false;
    view_get_setting(app, view_id, ViewSetting_ShowWhitespace, &show_whitespace);
    if (show_whitespace){
        if (token_array.tokens == 0){
            draw_whitespace_highlight(app, buffer, text_layout_id, cursor_roundness);
        }
        else{
            draw_whitespace_highlight(app, text_layout_id, &token_array, cursor_roundness);
        }
    }
    // NOTE(allen): Cursor
    switch (fcoder_mode){
        case FCoderMode_Original:
        {
            draw_original_4coder_style_cursor_mark_highlight(app, view_id, is_active_view, buffer, text_layout_id, cursor_roundness, mark_thickness);
        }break;
        case FCoderMode_NotepadLike:
        {
            draw_notepad_style_cursor_highlight(app, view_id, buffer, text_layout_id, cursor_roundness);
#if BIG_CURSOR
            if(cursor_pos == mark_pos)
                draw_character_block(app, text_layout_id, 
                                     {cursor_pos, cursor_pos + 1}, 0, 
                                     finalize_color(defcolor_cursor, 0));
#endif
        }break;
    }
    // NOTE(allen): Fade ranges
    paint_fade_ranges(app, text_layout_id, buffer);
    
    // NOTE(allen): put the actual text on the actual screen
    draw_text_layout_default(app, text_layout_id);
    
    draw_set_clip(app, prev_clip);
}



function void
krz_draw_file_bar(Application_Links *app, View_ID view_id, Buffer_ID buffer, Face_ID face_id, Rect_f32 bar){
    Scratch_Block scratch(app);
    
    draw_rectangle_fcolor(app, bar, 0.f, fcolor_id(defcolor_bar));
    
    FColor base_color = fcolor_id(defcolor_base);
    FColor pop2_color = fcolor_id(defcolor_pop2);
    
    i64 cursor_position = view_get_cursor_pos(app, view_id);
    Buffer_Cursor cursor = view_compute_cursor(app, view_id, seek_pos(cursor_position));
    
    Fancy_Line list = {};
    String_Const_u8 unique_name = push_buffer_unique_name(app, scratch, buffer);
    push_fancy_string(scratch, &list, base_color, unique_name);
    push_fancy_stringf(scratch, &list, base_color, " - Row: %3.lld Col: %3.lld -", cursor.line, cursor.col);
    
    Managed_Scope scope = buffer_get_managed_scope(app, buffer);
    Line_Ending_Kind *eol_setting = scope_attachment(app, scope, buffer_eol_setting,
                                                     Line_Ending_Kind);
    switch (*eol_setting){
        case LineEndingKind_Binary:
        {
            push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" bin"));
        }break;
        
        case LineEndingKind_LF:
        {
            push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" lf"));
        }break;
        
        case LineEndingKind_CRLF:
        {
            push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" crlf"));
        }break;
    }
    
    u8 space[3];
    {
        Dirty_State dirty = buffer_get_dirty_state(app, buffer);
        String_u8 str = Su8(space, 0, 3);
        if (dirty != 0){
            string_append(&str, string_u8_litexpr(" "));
        }
        if (HasFlag(dirty, DirtyState_UnsavedChanges)){
            string_append(&str, string_u8_litexpr("*"));
        }
        if (HasFlag(dirty, DirtyState_UnloadedChanges)){
            string_append(&str, string_u8_litexpr("!"));
        }
        push_fancy_string(scratch, &list, pop2_color, str.string);
    }
    
    if(case_sensitive_mode)
    {
        push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" (Alt C)ase sensitive"));
    }
    
    Vec2_f32 p = bar.p0 + V2f32(2.f, 2.f);
    draw_fancy_line(app, face_id, fcolor_zero(), &list, p);
}

function Rect_f32
krz_draw_background_and_margin(Application_Links *app, View_ID view, 
                               Buffer_ID buffer, b32 is_active_view){
    FColor margin_color = get_panel_margin_color(is_active_view?UIHighlight_Active:UIHighlight_None);
    
    Scratch_Block scratch(app);
    String_Const_u8 string = push_buffer_base_name(app, scratch, buffer);
    
    b32 matches = string_match(string, string_u8_litexpr("*compilation*"));
    
    FColor back = fcolor_id(defcolor_back);
    if(matches) back = fcolor_id(compilation_buffer_color);
    
    return(draw_background_and_margin(app, view, margin_color, back, margin_width));
}

function void
krz_render_caller(Application_Links *app, Frame_Info frame_info, View_ID view_id){
    ProfileScope(app, "default render caller");
    View_ID active_view = get_active_view(app, Access_Always);
    b32 is_active_view = (active_view == view_id);
    Buffer_ID buffer = view_get_buffer(app, view_id, Access_Always);
    
    Rect_f32 region = krz_draw_background_and_margin(app, view_id, buffer, is_active_view);
    Rect_f32 prev_clip = draw_set_clip(app, region);
    
    
    Face_ID face_id = get_face_id(app, buffer);
    Face_Metrics face_metrics = get_face_metrics(app, face_id);
    f32 line_height = face_metrics.line_height;
    f32 digit_advance = face_metrics.decimal_digit_advance;
    
    // NOTE(allen): file bar
    b64 showing_file_bar = false;
    if (view_get_setting(app, view_id, ViewSetting_ShowFileBar, &showing_file_bar) && showing_file_bar){
#if BAR_POSITION_BOT
        // mouse fix in krz_fix_view_pos_from_xy
        Rect_f32_Pair pair = layout_file_bar_on_bot(region, line_height);
        krz_draw_file_bar(app, view_id, buffer, face_id, pair.max);
        region = pair.min;
#else
        Rect_f32_Pair pair = layout_file_bar_on_top(region, line_height);
        krz_draw_file_bar(app, view_id, buffer, face_id, pair.min);
        region = pair.max;
#endif
    }
    
    Buffer_Scroll scroll = view_get_buffer_scroll(app, view_id);
    
    Buffer_Point_Delta_Result delta = delta_apply(app, view_id,
                                                  frame_info.animation_dt, scroll);
    if (!block_match_struct(&scroll.position, &delta.point)){
        block_copy_struct(&scroll.position, &delta.point);
        view_set_buffer_scroll(app, view_id, scroll, SetBufferScroll_NoCursorChange);
    }
    if (delta.still_animating){
        animate_in_n_milliseconds(app, 0);
    }
    
    // NOTE(allen): query bars
    region = default_draw_query_bars(app, region, view_id, face_id);
    
    // NOTE(allen): FPS hud
    if (show_fps_hud){
        Rect_f32_Pair pair = layout_fps_hud_on_bottom(region, line_height);
        draw_fps_hud(app, frame_info, face_id, pair.max);
        region = pair.min;
        animate_in_n_milliseconds(app, 1000);
    }
    
    // NOTE(allen): layout line numbers
    Rect_f32 line_number_rect = {};
    if (global_config.show_line_number_margins){
        Rect_f32_Pair pair = layout_line_number_margin(app, buffer, region, digit_advance);
        line_number_rect = pair.min;
        region = pair.max;
    }
    
    // NOTE(allen): begin buffer render
    Buffer_Point buffer_point = scroll.position;
    Text_Layout_ID text_layout_id = text_layout_create(app, buffer, region, buffer_point);
    
    vertical_scope_annotation_draw(app, view_id, buffer, text_layout_id, vertical_scope_annotation_flag_bottom_to_top |
                                   vertical_scope_annotation_flag_highlight);
    
    // NOTE(allen): draw line numbers
    if (global_config.show_line_number_margins){
        draw_line_number_margin(app, view_id, buffer, face_id, text_layout_id, line_number_rect);
    }
    
    // NOTE(allen): draw the buffer
    krz_render_buffer(app, view_id, face_id, buffer, text_layout_id, region);
    
    text_layout_free(app, text_layout_id);
    draw_set_clip(app, prev_clip);
    
    
}

CUSTOM_COMMAND_SIG(krz_startup)
CUSTOM_DOC("Default command for responding to a startup event")
{
    ProfileScope(app, "default startup");
    User_Input input = get_current_input(app);
    if (match_core_code(&input, CoreCode_Startup)){
        String_Const_u8_Array file_names = input.event.core.file_names;
        load_themes_default_folder(app);
        default_4coder_initialize(app, file_names);
        // default_4coder_side_by_side_panels(app);
        default_4coder_one_panel(app, buffer_identifier(string_u8_litexpr("*scratch*")));
        if (global_config.automatically_load_project){
            load_project(app);
        }
    }
}

CUSTOM_COMMAND_SIG(split_fullscreen_mode)
CUSTOM_DOC("Toggle with 2 modes")
{
    if(!is_fullscreen_mode)
    {
        // global_config.show_line_number_margins = 1;
        // NOTE: Open second panel
        View_ID view = get_active_view(app, Access_Always);
        View_ID new_view = open_view(app, view, ViewSplit_Right);
        new_view_settings(app, new_view);
        Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
        view_set_buffer(app, new_view, buffer, 0);
    }
    else
    {
        // global_config.show_line_number_margins = 0;
        // NOTE: Close second panel
        View_ID view = get_active_view(app, Access_Always);
        get_next_view_looped_all_panels(app, view, Access_Always);
        view_close(app, view);
    }
    
    is_fullscreen_mode = !is_fullscreen_mode;
    toggle_fullscreen(app);
}

function Rect_f32
krz_buffer_region(Application_Links *app, View_ID view_id, Rect_f32 region){
    Buffer_ID buffer = view_get_buffer(app, view_id, Access_Always);
    Face_ID face_id = get_face_id(app, buffer);
    Face_Metrics metrics = get_face_metrics(app, face_id);
    f32 line_height = metrics.line_height;
    f32 digit_advance = metrics.decimal_digit_advance;
    
    // NOTE(allen): margins
    region = rect_inner(region, margin_width);
    
    // NOTE(allen): file bar
    b64 showing_file_bar = false;
    if (view_get_setting(app, view_id, ViewSetting_ShowFileBar, &showing_file_bar) &&
        showing_file_bar){
        Rect_f32_Pair pair = layout_file_bar_on_top(region, line_height);
        region = pair.max;
    }
    
    // NOTE(allen): query bars
    {
        Query_Bar *space[32];
        Query_Bar_Ptr_Array query_bars = {};
        query_bars.ptrs = space;
        if (get_active_query_bars(app, view_id, ArrayCount(space), &query_bars)){
            Rect_f32_Pair pair = layout_query_bar_on_top(region, line_height, query_bars.count);
            region = pair.max;
        }
    }
    
    // NOTE(allen): FPS hud
    if (show_fps_hud){
        Rect_f32_Pair pair = layout_fps_hud_on_bottom(region, line_height);
        region = pair.min;
    }
    
    // NOTE(allen): line numbers
    if (global_config.show_line_number_margins){
        Rect_f32_Pair pair = layout_line_number_margin(app, buffer, region, digit_advance);
        region = pair.max;
    }
    
    return(region);
}

function i64
krz_boundary_inside_left_paren(Application_Links *app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos){
    function Character_Predicate predicate = {};
    
    predicate = character_predicate_from_character((u8)'(');
    predicate = character_predicate_not(&predicate);
    
    i64 result = buffer_seek_character_class_change_0_1(app, buffer, &predicate, direction, pos);
    
    return(result);
}

function i64
krz_boundary_inside_right_paren(Application_Links *app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos){
    function Character_Predicate predicate = {};
    
    predicate = character_predicate_from_character((u8)')');
    predicate = character_predicate_not(&predicate);
    
    i64 result = 0;
    result = buffer_seek_character_class_change_1_0(app, buffer, &predicate, direction, pos);
    
    return(result);
}

CUSTOM_COMMAND_SIG(write_warning)
{
    write_named_comment_string(app, "WARNING");
}

CUSTOM_COMMAND_SIG(write_fixme)
{
    write_named_comment_string(app, "FIXME");
}

CUSTOM_COMMAND_SIG(krz_move_right_alpha_numeric_boundary)
{
    Scratch_Block scratch(app);
    current_view_scan_move(app, Scan_Forward, push_boundary_list(scratch, boundary_alpha_numeric, boundary_line, krz_boundary_inside_left_paren, krz_boundary_inside_right_paren));
}

CUSTOM_COMMAND_SIG(krz_move_left_alpha_numeric_boundary)
{
    Scratch_Block scratch(app);
    current_view_scan_move(app, Scan_Backward, push_boundary_list(scratch, boundary_alpha_numeric, boundary_line, krz_boundary_inside_left_paren, krz_boundary_inside_right_paren));
}

CUSTOM_COMMAND_SIG(krz_move_right_whitespace_boundary_or_line_boundary)
{
    Scratch_Block scratch(app);
    current_view_scan_move(app, Scan_Forward,
                           push_boundary_list(scratch, boundary_non_whitespace, 
                                              boundary_line));
}

CUSTOM_COMMAND_SIG(krz_move_left_whitespace_boundary_or_line_boundary)
{
    Scratch_Block scratch(app);
    current_view_scan_move(app, Scan_Backward,
                           push_boundary_list(scratch, boundary_non_whitespace, boundary_line));
}

// Modified for F5 to F8 keys
// index 1 == F5
CUSTOM_COMMAND_SIG(krz_project_fkey_command)
{
    ProfileScope(app, "project fkey command");
    User_Input input = get_current_input(app);
    b32 got_ind = false;
    i32 ind = 0;
    if (input.event.kind == InputEventKind_KeyStroke){
        if (KeyCode_F5 <= input.event.key.code && input.event.key.code <= KeyCode_F16){
            ind = (input.event.key.code - KeyCode_F5);
            got_ind = true;
        }
        else if (KeyCode_1 <= input.event.key.code && input.event.key.code <= KeyCode_9){
            ind = (input.event.key.code - '1');
            got_ind = true;
        }
        else if (input.event.key.code == KeyCode_0){
            ind = 9;
            got_ind = true;
        }
        if (got_ind){
            exec_project_fkey_command(app, ind);
        }
    }
}

CUSTOM_UI_COMMAND_SIG(krz_snippet_lister)
CUSTOM_DOC("Opens a snippet lister for inserting whole pre-written snippets of text.")
{
    View_ID view = get_this_ctx_view(app, Access_ReadWrite);
    if (view != 0){
        Snippet *snippet = get_snippet_from_user(app, krz_snippets,
                                                 ArrayCount(krz_snippets),
                                                 "Snippet:");
        
        Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
        i64 pos = view_get_cursor_pos(app, view);
        write_snippet(app, view, buffer, pos, snippet);
    }
}

CUSTOM_COMMAND_SIG(painter_mode_switch)
CUSTOM_DOC("Painter Mode Switch !")
{
    painter_mode = !painter_mode;
}

CUSTOM_COMMAND_SIG(painter_mode_clear)
CUSTOM_DOC("Clear the paint")
{
    brush_strokes_size = 0;
}

CUSTOM_COMMAND_SIG(painter_mode_brush_size_lower)
CUSTOM_DOC("Brush size lower")
{
    if (brush_size > brush_size_control) brush_size -= brush_size_control;
}

CUSTOM_COMMAND_SIG(painter_mode_brush_size_upper)
CUSTOM_DOC("Brush size upper")
{
    brush_size += brush_size_control;
}



void
custom_layer_init(Application_Links *app){
    Thread_Context *tctx = get_thread_context(app);
    
    // NOTE(allen): setup for default framework
    async_task_handler_init(app, &global_async_system);
    clipboard_init(get_base_allocator_system(), /*history_depth*/ 64, &clipboard0);
    code_index_init();
    buffer_modified_set_init();
    Profile_Global_List *list = get_core_profile_list(app);
    ProfileThreadName(tctx, list, string_u8_litexpr("main"));
    initialize_managed_id_metadata(app);
    set_default_color_scheme(app);
    heap_init(&global_heap, tctx->allocator);
    global_config_arena = make_arena_system();
    fade_range_arena = make_arena_system(KB(8));
    
    // NOTE(allen): default hooks and command maps
    set_all_default_hooks(app);
    
    set_custom_hook(app, HookID_RenderCaller, krz_render_caller);
    set_custom_hook(app, HookID_BufferRegion, krz_buffer_region);
    set_custom_hook(app, HookID_WholeScreenRenderCaller, painter_whole_screen_render_caller);
    
    brush_strokes = (brush_in_time *)heap_allocate(&global_heap, sizeof(brush_in_time) * max_size_of_array);
#include "key_bindings.cpp"
}

#endif //FCODER_DEFAULT_BINDINGS

// BOTTOM
