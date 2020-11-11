/*
4coder_default_bidings.cpp - Supplies the default bindings used for default 4coder behavior.
*/

/* TODO:
    * Some ways to navigate through the mess
    * Fix move lines at the edge of buffer
    * find all @ find all TODOs etc.
    * Close lister with control W
 */

// TOP
#define FCODER_MODE_ORIGINAL 0
#define BIG_CURSOR 1
#define HIGHLIGH_SELECTION_MATCH 1

#define DARK0H	0xff1d2021
#define DARK0	0xff282828
#define DARK0S	0xff32302f
#define DARK1	0xff3c3836
#define DARK2	0xff504945
#define DARK3	0xff665c54
#define DARK4	0xff7c6f64
#define LIGHT0H	0xfff9f5d7
#define LIGHT0	0xfffbf1c7
#define LIGHT0S	0xfff2e5bc
#define LIGHT1	0xffebdbb2
#define LIGHT2	0xffd5c4a1
#define LIGHT3	0xffbdae93
#define LIGHT4	0xffa89984
#define GRAY	0xff928374
#define RED0	0xfffb4934
#define RED1	0xff9d0006
#define GREEN0	0xffb8bb26
#define GREEN1	0xff79740e
#define YELLOW0	0xfffabd2f
#define YELLOW1	0xffb57614
#define BLUE0	0xff83a598
#define BLUE1	0xff076678
#define PURPLE0	0xffd3869b
#define PURPLE1	0xff8f3f71
#define AQUA0	0xff8ec07c
#define AQUA1	0xff427b58
#define ORANGE0	0xfffe8019
#define ORANGE1	0xffaf3a03

#if !defined(FCODER_DEFAULT_BINDINGS_CPP)
#define FCODER_DEFAULT_BINDINGS_CPP

#include "4coder_default_include.cpp"

// NOTE(allen): Users can declare their own managed IDs here.
// function void vertical_scope_annotation_draw( Application_Links *app, View_ID view, Buffer_ID buffer, Text_Layout_ID text_layout_id, u32 flags );

global bool case_sensitive_mode = false;
global bool is_fullscreen_mode;
global f32 margin_width = 0.f;
global i32 max_selection_size = 128;

CUSTOM_ID( command_map, mapid_krz_mode );
CUSTOM_ID( colors, selection_match_highlight );
CUSTOM_ID( colors, primitive_highlight_type );
CUSTOM_ID( colors, primitive_highlight_function );
CUSTOM_ID( colors, primitive_highlight_macro );
CUSTOM_ID( colors, primitive_highlight_4coder_command );
CUSTOM_ID( colors, vertical_scope_annotation_text );
CUSTOM_ID( colors, vertical_scope_annotation_highlight );

#if !defined(META_PASS)
#include "generated/managed_id_metadata.cpp"
#endif

#include "primitive_highlight.cpp"
#include "vertical_scope_annotations.cpp"
#include "selection_based_cursor_improvements.cpp"

function void
krz_isearch_case_mode_aware(Application_Links *app, Scan_Direction start_scan, i64 first_pos,
                            String_Const_u8 query_init){
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    if (!buffer_exists(app, buffer)){
        return;
    }
    
    i64 buffer_size = buffer_get_size(app, buffer);
    
    Query_Bar_Group group(app);
    Query_Bar bar = {};
    if (start_query_bar(app, &bar, 0) == 0){
        return;
    }
    
    Vec2_f32 old_margin = {};
    Vec2_f32 old_push_in = {};
    view_get_camera_bounds(app, view, &old_margin, &old_push_in);
    
    Vec2_f32 margin = old_margin;
    margin.y = clamp_bot(200.f, margin.y);
    view_set_camera_bounds(app, view, margin, old_push_in);
    
    Scan_Direction scan = start_scan;
    i64 pos = first_pos;
    
    u8 bar_string_space[256];
    bar.string = SCu8(bar_string_space, query_init.size);
    block_copy(bar.string.str, query_init.str, query_init.size);
    
    String_Const_u8 isearch_str = string_u8_litexpr("I-Search: ");
    String_Const_u8 rsearch_str = string_u8_litexpr("Reverse-I-Search: ");
    
    u64 match_size = bar.string.size;
    
    User_Input in = {};
    for (;;){
        switch (scan){
            case Scan_Forward:
            {
                bar.prompt = isearch_str;
            }break;
            case Scan_Backward:
            {
                bar.prompt = rsearch_str;
            }break;
        }
        isearch__update_highlight(app, view, Ii64_size(pos, match_size));
        
        in = get_next_input(app, EventPropertyGroup_Any, EventProperty_Escape);
        if (in.abort){
            break;
        }
        
        String_Const_u8 string = to_writable(&in);
        
        b32 string_change = false;
        if (match_key_code(&in, KeyCode_Return) ||
            match_key_code(&in, KeyCode_Tab)){
            Input_Modifier_Set *mods = &in.event.key.modifiers;
            if (has_modifier(mods, KeyCode_Control)){
                bar.string.size = cstring_length(previous_isearch_query);
                block_copy(bar.string.str, previous_isearch_query, bar.string.size);
            }
            else{
                u64 size = bar.string.size;
                size = clamp_top(size, sizeof(previous_isearch_query) - 1);
                block_copy(previous_isearch_query, bar.string.str, size);
                previous_isearch_query[size] = 0;
                break;
            }
        }
        else if (string.str != 0 && string.size > 0){
            String_u8 bar_string = Su8(bar.string, sizeof(bar_string_space));
            string_append(&bar_string, string);
            bar.string = bar_string.string;
            string_change = true;
        }
        else if (match_key_code(&in, KeyCode_Backspace)){
            if (is_unmodified_key(&in.event)){
                u64 old_bar_string_size = bar.string.size;
                bar.string = backspace_utf8(bar.string);
                string_change = (bar.string.size < old_bar_string_size);
            }
            else if (has_modifier(&in.event.key.modifiers, KeyCode_Control)){
                if (bar.string.size > 0){
                    string_change = true;
                    bar.string.size = 0;
                }
            }
        }
        
        b32 do_scan_action = false;
        b32 do_scroll_wheel = false;
        Scan_Direction change_scan = scan;
        if (!string_change){
            if (match_key_code(&in, KeyCode_PageDown) ||
                match_key_code(&in, KeyCode_Down)){
                change_scan = Scan_Forward;
                do_scan_action = true;
            }
            else if (match_key_code(&in, KeyCode_PageUp) ||
                     match_key_code(&in, KeyCode_Up)){
                change_scan = Scan_Backward;
                do_scan_action = true;
            }
            else{
                // NOTE(allen): is the user trying to execute another command?
                View_Context ctx = view_current_context(app, view);
                Mapping *mapping = ctx.mapping;
                Command_Map *map = mapping_get_map(mapping, ctx.map_id);
                Command_Binding binding = map_get_binding_recursive(mapping, map, &in.event);
                if (binding.custom != 0){
                    if (binding.custom == search){
                        change_scan = Scan_Forward;
                        do_scan_action = true;
                    }
                    else if (binding.custom == reverse_search){
                        change_scan = Scan_Backward;
                        do_scan_action = true;
                    }
                    else{
                        Command_Metadata *metadata = get_command_metadata(binding.custom);
                        if (metadata != 0){
                            if (metadata->is_ui){
                                view_enqueue_command_function(app, view, binding.custom);
                                break;
                            }
                        }
                        binding.custom(app);
                    }
                }
                else{
                    leave_current_input_unhandled(app);
                }
            }
        }
        
        if (string_change){
            switch (scan){
                case Scan_Forward:
                {
                    i64 new_pos = 0;
                    if(case_sensitive_mode) seek_string_forward(app, buffer, pos - 1, 0, bar.string, &new_pos);
                    else seek_string_insensitive_forward(app, buffer, pos - 1, 0, bar.string, &new_pos);
                    if (new_pos < buffer_size){
                        pos = new_pos;
                        match_size = bar.string.size;
                    }
                }break;
                
                case Scan_Backward:
                {
                    i64 new_pos = 0;
                    if(case_sensitive_mode) seek_string_backward(app, buffer, pos + 1, 0, bar.string, &new_pos);
                    else seek_string_insensitive_backward(app, buffer, pos + 1, 0, bar.string, &new_pos);
                    if (new_pos >= 0){
                        pos = new_pos;
                        match_size = bar.string.size;
                    }
                }break;
            }
        }
        else if (do_scan_action){
            scan = change_scan;
            switch (scan){
                case Scan_Forward:
                {
                    i64 new_pos = 0;
                    if(case_sensitive_mode) seek_string_forward(app, buffer, pos, 0, bar.string, &new_pos);
                    else seek_string_insensitive_forward(app, buffer, pos, 0, bar.string, &new_pos);
                    if (new_pos < buffer_size){
                        pos = new_pos;
                        match_size = bar.string.size;
                    }
                }break;
                
                case Scan_Backward:
                {
                    i64 new_pos = 0;
                    if(case_sensitive_mode) seek_string_backward(app, buffer, pos, 0, bar.string, &new_pos);
                    else seek_string_insensitive_backward(app, buffer, pos, 0, bar.string, &new_pos);
                    if (new_pos >= 0){
                        pos = new_pos;
                        match_size = bar.string.size;
                    }
                }break;
            }
        }
        else if (do_scroll_wheel){
            mouse_wheel_scroll(app);
        }
    }
    
    view_disable_highlight_range(app, view);
    
    if (in.abort){
        u64 size = bar.string.size;
        size = clamp_top(size, sizeof(previous_isearch_query) - 1);
        block_copy(previous_isearch_query, bar.string.str, size);
        previous_isearch_query[size] = 0;
        view_set_cursor_and_preferred_x(app, view, seek_pos(first_pos));
    }
    
    view_set_camera_bounds(app, view, old_margin, old_push_in);
}

CUSTOM_COMMAND_SIG(search_selection_or_identifier)
{
    View_ID view = get_active_view(app, Access_Always);
	Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    
	i64 cursor_pos = view_get_cursor_pos(app, view);
	i64 mark_pos = view_get_mark_pos(app, view);
    
	i64 min_pos = Min(cursor_pos, mark_pos);
	i64 max_pos = Max(cursor_pos, mark_pos);
    
    Scratch_Block scratch(app);
    i64 selection = max_pos - min_pos;
    if(selection == 0 || selection > max_selection_size)
    {
        Range_i64 range = enclose_pos_alpha_numeric_underscore(app, buffer, cursor_pos);
        
        String_Const_u8 query = push_buffer_range(app, scratch, buffer, range);
        
        krz_isearch_case_mode_aware(app, Scan_Forward, range.first, query);
    }
    else
    {
        Range_i64 range = {};
        range.start = min_pos;
        range.end = max_pos;
        
        String_Const_u8 query = push_buffer_range(app, scratch, buffer, range);
        
        krz_isearch_case_mode_aware(app, Scan_Forward, range.first, query);
    }
}

function void
krz_query_replace_base(Application_Links *app, View_ID view, Buffer_ID buffer_id, i64 pos, String_Const_u8 r, String_Const_u8 w){
    i64 new_pos = 0;
    
    if(case_sensitive_mode) seek_string_forward(app, buffer_id, pos - 1, 0, r, &new_pos);
    else seek_string_insensitive_forward(app, buffer_id, pos - 1, 0, r, &new_pos);
    
    User_Input in = {};
    for (;;){
        Range_i64 match = Ii64(new_pos, new_pos + r.size);
        isearch__update_highlight(app, view, match);
        
        in = get_next_input(app, EventProperty_AnyKey, EventProperty_MouseButton);
        if (in.abort || match_key_code(&in, KeyCode_Escape) || !is_unmodified_key(&in.event)){
            break;
        }
        
        i64 size = buffer_get_size(app, buffer_id);
        if (match.max <= size &&
            (match_key_code(&in, KeyCode_D) ||
             match_key_code(&in, KeyCode_Return) ||
             match_key_code(&in, KeyCode_Tab))){
            buffer_replace_range(app, buffer_id, match, w);
            pos = match.start + w.size;
        }
        else if(match_key_code(&in, KeyCode_Up))
        {
            pos = match.start - w.size;
            if(case_sensitive_mode) seek_string_backward(app, buffer_id, pos, 0, r, &new_pos);
            else seek_string_insensitive_backward(app, buffer_id, pos, 0, r, &new_pos);
            continue;
        }
        else{
            pos = match.max;
        }
        
        if(case_sensitive_mode) seek_string_forward(app, buffer_id, pos, 0, r, &new_pos);
        else seek_string_insensitive_forward(app, buffer_id, pos, 0, r, &new_pos);
    }
    
    view_disable_highlight_range(app, view);
    
    if (in.abort){
        return;
    }
    
    view_set_cursor_and_preferred_x(app, view, seek_pos(pos));
}

function void
krz_query_replace_parameter(Application_Links *app, String_Const_u8 replace_str, i64 start_pos, b32 add_replace_query_bar){
    Query_Bar_Group group(app);
    Query_Bar replace = {};
    replace.prompt = string_u8_litexpr("Replace: ");
    replace.string = replace_str;
    
    if (add_replace_query_bar){
        start_query_bar(app, &replace, 0);
    }
    
    Query_Bar with = {};
    u8 with_space[1024];
    with.prompt = string_u8_litexpr("With: ");
    with.string = SCu8(with_space, (u64)0);
    with.string_capacity = sizeof(with_space);
    
    if (query_user_string(app, &with)){
        String_Const_u8 r = replace.string;
        String_Const_u8 w = with.string;
        
        View_ID view = get_active_view(app, Access_ReadVisible);
        Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
        i64 pos = start_pos;
        
        Query_Bar bar = {};
        bar.prompt = string_u8_litexpr("Replace? (D)yes, (n)ext(arrows too), (esc)\n");
        start_query_bar(app, &bar, 0);
        
        krz_query_replace_base(app, view, buffer, pos, r, w);
    }
}

CUSTOM_COMMAND_SIG(krz_query_replace)
CUSTOM_DOC("Queries the user for two strings, and incrementally replaces every occurence of the first string with the second string.")
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    if (buffer != 0){
        Query_Bar_Group group(app);
        Query_Bar replace = {};
        u8 replace_space[1024];
        replace.prompt = string_u8_litexpr("Replace: ");
        replace.string = SCu8(replace_space, (u64)0);
        replace.string_capacity = sizeof(replace_space);
        if (query_user_string(app, &replace)){
            if (replace.string.size > 0){
                i64 pos = view_get_cursor_pos(app, view);
                krz_query_replace_parameter(app, replace.string, pos, false);
            }
        }
    }
}

CUSTOM_COMMAND_SIG(query_replace_selection_or_identifier)
{
    View_ID view = get_active_view(app, Access_Always);
	Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    if(buffer == 0) return;
    
	i64 cursor_pos = view_get_cursor_pos(app, view);
	i64 mark_pos = view_get_mark_pos(app, view);
    
	i64 min_pos = Min(cursor_pos, mark_pos);
	i64 max_pos = Max(cursor_pos, mark_pos);
    
    Scratch_Block scratch(app);
    i64 selection = max_pos - min_pos;
    
    if(selection == 0 || selection > max_selection_size)
    {
        Range_i64 range = enclose_pos_alpha_numeric_underscore(app, buffer, cursor_pos);
        String_Const_u8 replace = push_buffer_range(app, scratch, buffer, range);
        if (replace.size != 0){
            krz_query_replace_parameter(app, replace, range.min, true);
        }
        else{
            krz_query_replace(app);
        }
    }
    else
    {
        Range_i64 range = {};
        range.start = min_pos;
        range.end = max_pos;
        
        String_Const_u8 replace = push_buffer_range(app, scratch, buffer, range);
        if (replace.size != 0){
            krz_query_replace_parameter(app, replace, range.min, true);
        }
    }
    
    view_set_cursor(app, view, seek_pos(cursor_pos));
}

CUSTOM_COMMAND_SIG(list_all_locations_of_identifier_or_selection)
{
    View_ID view = get_active_view(app, Access_Always);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    
    i64 cursor_pos = view_get_cursor_pos(app, view);
    i64 mark_pos = view_get_mark_pos(app, view);
    
    i64 min_pos = Min(cursor_pos, mark_pos);
    i64 max_pos = Max(cursor_pos, mark_pos);
    
    i64 selection = max_pos - min_pos;
    Range_i64 range = enclose_pos_alpha_numeric_underscore(app, buffer, cursor_pos);
    
    if(selection == 0)
    {
        if(range_size(range) == 0)
        {
            if(case_sensitive_mode) list_all_locations(app);
            else list_all_locations_case_insensitive(app);
        }
        else
        {
            if(case_sensitive_mode) list_all_locations_of_identifier(app);
            else list_all_locations_of_identifier_case_insensitive(app);
        } 
    }
    else
    {
        if(case_sensitive_mode) list_all_locations_of_selection(app);
        else list_all_locations_of_selection_case_insensitive(app);
    }
}

// Create a new clear buffer in the current view (without switching to the sidepanel)
function Buffer_ID
krz_create_or_switch_to_buffer_and_clear_by_name(Application_Links *app, String_Const_u8 name_string, View_ID default_target_view){
    Buffer_ID search_buffer = get_buffer_by_name(app, name_string, Access_Always);
    if (search_buffer != 0){
        buffer_set_setting(app, search_buffer, BufferSetting_ReadOnly, true);
        
        View_ID target_view = default_target_view;
        
        View_ID view_with_buffer_already_open = get_first_view_with_buffer(app, search_buffer);
        if (view_with_buffer_already_open != 0){
            target_view = view_with_buffer_already_open;
            // TODO(allen): there needs to be something like
            // view_exit_to_base_context(app, target_view);
            //view_end_ui_mode(app, target_view);
        }
        else{
            view_set_buffer(app, target_view, search_buffer, 0);
        }
        view_set_active(app, target_view);
        
        clear_buffer(app, search_buffer);
        buffer_send_end_signal(app, search_buffer);
    }
    else{
        search_buffer = create_buffer(app, name_string, BufferCreate_AlwaysNew);
        buffer_set_setting(app, search_buffer, BufferSetting_Unimportant, true);
        buffer_set_setting(app, search_buffer, BufferSetting_ReadOnly, true);
#if 0
        buffer_set_setting(app, search_buffer, BufferSetting_WrapLine, false);
#endif
        view_set_buffer(app, default_target_view, search_buffer, 0);
        // view_set_active(app, default_target_view);
    }
    
    return(search_buffer);
}


internal void
krz_list_all_locations__generic(Application_Links *app, String_Const_u8_Array needle, List_All_Locations_Flag flags){
    if (needle.count > 0){
        View_ID target_view = get_active_view(app, Access_Always);
        String_Match_Flag must_have_flags = 0;
        String_Match_Flag must_not_have_flags = 0;
        if (HasFlag(flags, ListAllLocationsFlag_CaseSensitive)){
            AddFlag(must_have_flags, StringMatch_CaseSensitive);
        }
        if (!HasFlag(flags, ListAllLocationsFlag_MatchSubstring)){
            AddFlag(must_not_have_flags, StringMatch_LeftSideSloppy);
            AddFlag(must_not_have_flags, StringMatch_RightSideSloppy);
        }
        
        Buffer_ID search_buffer = krz_create_or_switch_to_buffer_and_clear_by_name(app, search_name, target_view);
        print_all_matches_all_buffers(app, needle, must_have_flags, must_not_have_flags, search_buffer);
    }
}

internal void
krz_list_all_locations__generic(Application_Links *app, String_Const_u8 needle, List_All_Locations_Flag flags){
    if (needle.size != 0){
        String_Const_u8_Array array = {&needle, 1};
        list_all_locations__generic(app, array, flags);
    }
}

CUSTOM_COMMAND_SIG(TODOs_list)
CUSTOM_DOC("Todos lister")
{
    Scratch_Block scratch(app);
    String_Const_u8 query = push_u8_stringf(scratch, "TODO");
    View_ID original_view = get_active_view(app, Access_Always);
    Buffer_ID search_buffer = create_or_switch_to_buffer_and_clear_by_name(app, query, original_view);
    print_all_matches_all_buffers(app, query, StringMatch_CaseSensitive, 0, search_buffer);
    
    Heap *heap = &global_heap;
    View_ID view = get_active_view(app, Access_Always);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    Marker_List *list = get_or_make_list_for_buffer(app, heap, buffer);
    
    if (list != 0){
        view_set_active(app, original_view);
        Jump_Lister_Result jump = get_jump_index_from_user(app, list, "TODOs:");
        jump_to_jump_lister_result(app, view, list, &jump);
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
    
    // NOTE(allen): Token colorizing
    Token_Array token_array = get_token_array_from_buffer(app, buffer);
    if (token_array.tokens != 0){
        primitive_highlight_draw_cpp_token_colors( app, text_layout_id, &token_array, buffer );
        
        // NOTE(allen): Scan for TODOs and NOTEs
        if (global_config.use_comment_keyword){
            Comment_Highlight_Pair pairs[] = {
                {string_u8_litexpr("NOTE"), finalize_color(defcolor_comment_pop, 0)},
                {string_u8_litexpr("@return"), finalize_color(defcolor_comment_pop, 0)},
                {string_u8_litexpr("@arguments"), finalize_color(defcolor_comment_pop, 0)},
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
    
#if BIG_CURSOR
    if(cursor_pos == mark_pos)
        draw_character_block(app, text_layout_id, 
                             {cursor_pos, cursor_pos + 1}, 0, 
                             finalize_color(defcolor_cursor, 0));
#endif
    
#if HIGHLIGH_SELECTION_MATCH
    Scratch_Block scratch(app);
    
    Range_i64 selection = {Min(cursor_pos, mark_pos), Max(cursor_pos, mark_pos)};
    String_Const_u8 selected_string = push_buffer_range(app, scratch, buffer, selection);
    
    local_persist Character_Predicate *pred =
        &character_predicate_alpha_numeric_underscore_utf8;
    
    String_Match_List matches = buffer_find_all_matches(app, scratch, buffer,  0, visible_range, selected_string, pred, Scan_Forward);
    for (String_Match *node = matches.first; node != 0; node = node->next){
        draw_character_block(app, text_layout_id, 
                             node->range, 0, 
                             finalize_color(selection_match_highlight, 0));
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
krz_draw_background_and_margin(Application_Links *app, View_ID view, b32 is_active_view){
    FColor margin_color = get_panel_margin_color(is_active_view?UIHighlight_Active:UIHighlight_None);
    return(draw_background_and_margin(app, view, margin_color, fcolor_id(defcolor_back), margin_width));
}

function void
krz_render_caller(Application_Links *app, Frame_Info frame_info, View_ID view_id){
    ProfileScope(app, "default render caller");
    View_ID active_view = get_active_view(app, Access_Always);
    b32 is_active_view = (active_view == view_id);
    
    Rect_f32 region = krz_draw_background_and_margin(app, view_id, is_active_view);
    Rect_f32 prev_clip = draw_set_clip(app, region);
    
    Buffer_ID buffer = view_get_buffer(app, view_id, Access_Always);
    Face_ID face_id = get_face_id(app, buffer);
    Face_Metrics face_metrics = get_face_metrics(app, face_id);
    f32 line_height = face_metrics.line_height;
    f32 digit_advance = face_metrics.decimal_digit_advance;
    
    // NOTE(allen): file bar
    b64 showing_file_bar = false;
    if (view_get_setting(app, view_id, ViewSetting_ShowFileBar, &showing_file_bar) && showing_file_bar){
        
        // Rect_f32_Pair pair = layout_file_bar_on_top(region, line_height);
        // krz_draw_file_bar(app, view_id, buffer, face_id, pair.min);
        // region = pair.max;
        
        // mouse fix in krz_fix_view_pos_from_xy
        Rect_f32_Pair pair = layout_file_bar_on_bot(region, line_height);
        krz_draw_file_bar(app, view_id, buffer, face_id, pair.max);
        
        region = pair.min;
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

CUSTOM_COMMAND_SIG(write_comment_section)
{
    write_string(app, string_u8_litexpr("//------------------------- -------------------------\\\\"));
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

CUSTOM_COMMAND_SIG(krz_replace_in_all_buffers)
CUSTOM_DOC("Queries the user for a needle and string. Replaces all occurences of needle with string in all editable buffers.")
{
    global_history_edit_group_begin(app);
    
    Scratch_Block scratch(app);
    Query_Bar_Group group(app);
    String_Pair pair = query_user_replace_pair(app, scratch);
    Buffer_ID buffer = get_buffer_next(app, 0, Access_ReadWriteVisible);
    for (;;){
        // NOTE: bug was here in original, no check if valid
        if(buffer != 0 && pair.valid){
            
            buffer = get_buffer_next(app, buffer, Access_ReadWriteVisible);
            Range_i64 range = buffer_range(app, buffer);
            replace_in_range(app, buffer, range, pair.a, pair.b);
            
        }
        else break;
    }
    
    global_history_edit_group_end(app);
}

// FIXME: Very jump commands are very Buggy
// Idea: maybe find all "@" symbols in a file / project, these are bookmarks

struct jump_info
{
    i64 pos;
    Buffer_ID buffer;
};
global jump_info jump_table[4] = {};

#define setup_jump_command(number) \
CUSTOM_COMMAND_SIG(set_jump_table_##number) \
{ \
View_ID view = get_active_view(app, Access_ReadWriteVisible); \
Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible); \
i64 cursor_pos = view_get_cursor_pos(app, view); \
jump_table[number-1] = {cursor_pos, buffer}; \
} \
\
CUSTOM_COMMAND_SIG(goto_jump_table_##number) \
{ \
View_ID view = get_active_view(app, Access_ReadWriteVisible); \
if(jump_table[number-1].buffer != 0){ \
jump_to_location(app, view, jump_table[number-1].buffer, jump_table[number-1].pos); \
select_scope(app, view, {jump_table[number-1].pos, jump_table[number-1].pos}); \
} \
} \

// expands to goto_jump_table_1
//          and set_jump_table_1
setup_jump_command(1)
setup_jump_command(2)
setup_jump_command(3)
setup_jump_command(4)

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
#include "key_bindings.cpp"
}

#endif //FCODER_DEFAULT_BINDINGS

// BOTTOM
