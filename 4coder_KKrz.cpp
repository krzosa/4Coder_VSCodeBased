/*
4coder_default_bidings.cpp - Supplies the default bindings used for default 4coder behavior.
*/

/* TODO:
    * Some ways to navigate through the mess
    * Fix move lines at the edge of buffer
 */

// TOP
#define VERTICAL_BARS_SWITCH 1
#define PRIMITIVE_HIGHLIGHT_SWITCH 1
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

struct jump_info
{
    i64 pos;
    Buffer_ID buffer;
};
global jump_info jump_table[4] = {};

CUSTOM_ID( command_map, mapid_krz_mode );

void set_current_mapid( Application_Links* app, Command_Map_ID mapid ) {
    
    View_ID view = get_active_view( app, 0 );
    Buffer_ID buffer = view_get_buffer( app, view, 0 );
    Managed_Scope scope = buffer_get_managed_scope( app, buffer );
    Command_Map_ID* map_id_ptr = scope_attachment( app, scope, buffer_map_id, Command_Map_ID );
    *map_id_ptr = mapid;
}

CUSTOM_ID( colors, selection_match_highlight );

#if PRIMITIVE_HIGHLIGHT_SWITCH
CUSTOM_ID( colors, primitive_highlight_type );
CUSTOM_ID( colors, primitive_highlight_function );
CUSTOM_ID( colors, primitive_highlight_macro );
CUSTOM_ID( colors, primitive_highlight_4coder_command );
#endif

#if VERTICAL_BARS_SWITCH
CUSTOM_ID( colors, vertical_scope_annotation_text );
CUSTOM_ID( colors, vertical_scope_annotation_highlight );
typedef enum vertical_scope_annotation_flag_t {
    vertical_scope_annotation_flag_top_to_bottom = 0,
    vertical_scope_annotation_flag_bottom_to_top = 1,
    vertical_scope_annotation_flag_highlight = 1 << 1,
    vertical_scope_annotation_flag_highlight_case_label = 1 << 2,
    vertical_scope_annotation_flag_no_comment = 1 << 3,
    vertical_scope_annotation_flag_only_last_case = 1 << 4,
    
    /* NOTE: Do not use this. */
    vertical_scope_annotation_orientation_mask = vertical_scope_annotation_flag_bottom_to_top,
    
} vertical_scope_annotation_flag_t;

typedef struct vertical_scope_annotation_start_t {
    
    Token_Iterator_Array it;
    b32 contains_if;
    b32 contains_else;
    b32 is_valid;
    
} vertical_scope_annotation_start_t;
#endif

#if !defined(META_PASS)
#include "generated/managed_id_metadata.cpp"
#endif

struct selected_lines_info
{
    i64 min_pos;
    i64 max_pos;
    
    i64 min_line;
    i64 max_line;
    
    // all WHOLE selected lines
    Range_i64 entire_selected_lines_pos;
};

function selected_lines_info
get_selected_lines_info(Application_Links *app, View_ID view, Buffer_ID buffer)
{
    selected_lines_info result;
    
    i64 cursor_pos = view_get_cursor_pos(app, view);
	i64 mark_pos = view_get_mark_pos(app, view);
    
	result.min_pos = Min(cursor_pos, mark_pos);
	result.max_pos = Max(cursor_pos, mark_pos);
    
    result.min_line = get_line_number_from_pos(app, buffer, result.min_pos);
    result.max_line = get_line_number_from_pos(app, buffer, result.max_pos);
    
    i64 min_line = get_line_side_pos(app, buffer, result.min_line, Side_Min);
    i64 max_line = get_line_side_pos(app, buffer, result.max_line, Side_Max);
    
    result.entire_selected_lines_pos.min = min_line;
    result.entire_selected_lines_pos.max = max_line;
    
    return result;
}

CUSTOM_COMMAND_SIG(delete_selected_lines)
{
	View_ID view = get_active_view(app, Access_Always);
	Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    
    selected_lines_info selection = get_selected_lines_info(app, view, buffer);
    
    Range_i64 range = selection.entire_selected_lines_pos;
    
    range.start -= 1;
    range.start = clamp_bot(0, range.start);
    
    buffer_replace_range(app, buffer, range, string_u8_litexpr(""));
    
    view_set_cursor(app, view, seek_line_col(selection.min_line, 0));
    
}

CUSTOM_COMMAND_SIG(duplicate_multiple_lines_down)
{
    View_ID view = get_active_view(app, Access_Always);
	Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    
	selected_lines_info selection = get_selected_lines_info(app, view, buffer);
    Range_i64 range = selection.entire_selected_lines_pos;
    
    Scratch_Block scratch(app);
    
    String_Const_u8 string = push_buffer_range(app, scratch, buffer, range);
    string = push_u8_stringf(scratch, "%.*s\n%.*s", string_expand(string),
                             string_expand(string)); 
    buffer_replace_range(app, buffer, range, string);
    
    // NOTE(KKrzosa): Select the entire dupicated part
    i64 lines_duplicated = selection.max_line - selection.min_line;
    
    Range_i64 new_range;
    new_range.min = get_line_side_pos(app, buffer, selection.max_line + 1, Side_Min);
    new_range.max = get_line_side_pos(app, buffer, selection.max_line + lines_duplicated + 1, Side_Max);;
    
    if(lines_duplicated > 0) select_scope(app, view, new_range);
    else view_set_cursor(app, view, seek_line_col(selection.max_line + 1, 0));
}


CUSTOM_COMMAND_SIG(duplicate_multiple_lines_up)
{
    View_ID view = get_active_view(app, Access_Always);
	Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    
	selected_lines_info selection = get_selected_lines_info(app, view, buffer);
    
    Range_i64 range = selection.entire_selected_lines_pos;
    
    Scratch_Block scratch(app);
    
    String_Const_u8 string = push_buffer_range(app, scratch, buffer, range);
    string = push_u8_stringf(scratch, "%.*s\n%.*s", string_expand(string),
                             string_expand(string)); 
    buffer_replace_range(app, buffer, range, string);
    
    i64 lines_duplicated = selection.max_line - selection.min_line;
    
    if(lines_duplicated > 0) select_scope(app, view, range);
    else view_set_cursor(app, view, seek_line_col(selection.max_line, 0));
}

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

// QUERY REPLACE CASE SENSITIVE AND CASE INSENSITIVE



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

CUSTOM_COMMAND_SIG(move_lines_up)
CUSTOM_DOC("Move the lines up!")
{
	View_ID view = get_active_view(app, Access_Always);
	Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    
	History_Record_Index history_index = {};
    
	i64 cursor_pos = view_get_cursor_pos(app, view);
	i64 mark_pos = view_get_mark_pos(app, view);
    
	i64 min_pos = Min(cursor_pos, mark_pos);
	i64 max_pos = Max(cursor_pos, mark_pos);
    
	Buffer_Cursor min_cursor = view_compute_cursor(app, view, seek_pos(min_pos));
	Buffer_Cursor max_cursor = view_compute_cursor(app, view, seek_pos(max_pos));
    
	bool first = true;
	for (i64 line=min_cursor.line; line<=max_cursor.line; line++){
		view_set_cursor(app, view, seek_line_col(line, 0));
		current_view_move_line(app, Scan_Backward);
		if (first){
			history_index = buffer_history_get_current_state_index(app, buffer);
		}
		first = false;
	}
    
	Buffer_Cursor cursor_cursor = min_cursor;
	Buffer_Cursor mark_cursor = max_cursor;
	if (cursor_pos > mark_pos){
		cursor_cursor = max_cursor;
		mark_cursor = min_cursor;
	}
	view_set_cursor(app, view, seek_line_col(cursor_cursor.line-1, cursor_cursor.col));
	view_set_mark(app, view, seek_line_col(mark_cursor.line-1, mark_cursor.col));
	no_mark_snap_to_cursor(app, view);
    
	History_Record_Index history_index_end = buffer_history_get_current_state_index(app, buffer);
	buffer_history_merge_record_range(app, buffer, history_index, history_index_end, RecordMergeFlag_StateInRange_MoveStateForward);		
}

CUSTOM_COMMAND_SIG(move_lines_down)
CUSTOM_DOC("Move the lines down!")
{
	View_ID view = get_active_view(app, Access_Always);
	Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    
	History_Record_Index history_index = {};
    
	i64 cursor_pos = view_get_cursor_pos(app, view);
	i64 mark_pos = view_get_mark_pos(app, view);
    
	i64 min_pos = Min(cursor_pos, mark_pos);
	i64 max_pos = Max(cursor_pos, mark_pos);
    
	Buffer_Cursor min_cursor = view_compute_cursor(app, view, seek_pos(min_pos));
	Buffer_Cursor max_cursor = view_compute_cursor(app, view, seek_pos(max_pos));
    
	bool first = true;
	for (i64 line=max_cursor.line; line>=min_cursor.line; line--){
		view_set_cursor(app, view, seek_line_col(line, 0));
		current_view_move_line(app, Scan_Forward);
		if (first){
			history_index = buffer_history_get_current_state_index(app, buffer);
		}
		first = false;
	}
    
	Buffer_Cursor cursor_cursor = min_cursor;
	Buffer_Cursor mark_cursor = max_cursor;
	if (cursor_pos > mark_pos){
		cursor_cursor = max_cursor;
		mark_cursor = min_cursor;
	}
	view_set_cursor(app, view, seek_line_col(cursor_cursor.line+1, cursor_cursor.col));
	view_set_mark(app, view, seek_line_col(mark_cursor.line+1, mark_cursor.col));
	no_mark_snap_to_cursor(app, view);
    
	History_Record_Index history_index_end = buffer_history_get_current_state_index(app, buffer);
	buffer_history_merge_record_range(app, buffer, history_index, history_index_end, RecordMergeFlag_StateInRange_MoveStateForward);		
}



#if PRIMITIVE_HIGHLIGHT_SWITCH
/* NOTE: end is the index of the last item, NOT one past the last item. */
function void primitive_highlight_quick_sort_hashes_notes( u64* hashes, Code_Index_Note** notes, u64 start, u64 end ) {
    
    if ( hashes && start < end ) {
        
        u64 pivot_index = ( start + end ) / 2;
        u64 pivot_hash = hashes[ pivot_index ];
        
        u64 i = start, j = end;
        
        while ( 1 ) {
            
            while ( hashes[ i ] < pivot_hash ) {
                i++;
            }
            
            while ( hashes[ j ] > pivot_hash ) {
                j--;
            }
            
            if ( i < j ) {
                
                u64 hash_temp = hashes[ i ];
                hashes[ i ] = hashes[ j ];
                hashes[ j ] = hash_temp;
                
                Code_Index_Note* temp = notes[ i ];
                notes[ i ] = notes[ j ];
                notes[ j ] = temp;
                
                i++;
                j--;
                
            } else {
                
                break;
            }
        }
        
        primitive_highlight_quick_sort_hashes_notes( hashes, notes, start, j );
        primitive_highlight_quick_sort_hashes_notes( hashes, notes, j + 1, end );
    }
}

typedef struct primitive_highlight_hashes_notes_t {
    u64* hashes;
    Code_Index_Note** notes;
    i32 count;
} primitive_highlight_hashes_notes_t;

function primitive_highlight_hashes_notes_t primitive_highlight_create_big_note_array( Application_Links* app, Arena* arena ) {
    
    ProfileScope( app, "create_big_note_array" );
    
    primitive_highlight_hashes_notes_t hashes_notes = { 0 };
    
    Buffer_ID buffer_it = get_buffer_next( app, 0, Access_Always );
    
#if 0
    /* NOTE: locking the index only in this function seems to cost more than locking in the primitive_highlight_draw_cpp_token_colors function. */
    code_index_lock( );
#endif
    
    while ( buffer_it ) {
        
        Code_Index_File *file = code_index_get_file( buffer_it );
        
        if ( file ) {
            hashes_notes.count += file->note_array.count;
        }
        
        buffer_it = get_buffer_next( app, buffer_it, Access_Always );
    }
    hashes_notes.hashes = push_array( arena, u64, hashes_notes.count );
    hashes_notes.notes = push_array( arena, Code_Index_Note*, hashes_notes.count );
    
    i32 count = 0;
    
    {
        ProfileScope( app, "create hashes" );
        
        buffer_it = get_buffer_next( app, 0, Access_Always );
        
        while ( buffer_it ) {
            
            Code_Index_File *file = code_index_get_file( buffer_it );
            
            if ( file ) {
                
                for ( i32 i = 0; i < file->note_array.count; i++ ) {
                    hashes_notes.notes[ count ] = file->note_array.ptrs[ i ];
                    hashes_notes.hashes[ count ] = table_hash_u8( hashes_notes.notes[ count ]->text.str, hashes_notes.notes[ count ]->text.size );
                    count++;
                }
            }
            
            buffer_it = get_buffer_next( app, buffer_it, Access_Always );
        }
    }
    
#if 0
    code_index_unlock( );
#endif
    
    if ( count ) {
        ProfileScope( app, "quick_sort_hashes_notes" );
        primitive_highlight_quick_sort_hashes_notes( hashes_notes.hashes, hashes_notes.notes, 0, count - 1 );
    }
    
    return hashes_notes;
}

function Code_Index_Note* primitive_highlight_get_note( Application_Links* app, primitive_highlight_hashes_notes_t* hashes_notes, String_Const_u8 name ) {
    
    u8 type_weight[ 4 ];
    type_weight[ CodeIndexNote_4coderCommand ] = 1;
    type_weight[ CodeIndexNote_Function ] = 2;
    type_weight[ CodeIndexNote_Type ] = 3;
    type_weight[ CodeIndexNote_Macro ] = 4;
    
    ProfileScope( app, "get_note" );
    
    Code_Index_Note* result = 0;
    
    u64 name_hash = table_hash_u8( name.str, name.size );
    
    i32 start = 0;
    i32 end = hashes_notes->count - 1;
    
    while ( start <= end ) {
        
        i32 midle = ( start + end ) / 2;
        
        u64 note_hash = hashes_notes->hashes[ midle ];
        
        if ( name_hash < note_hash ) {
            end = midle - 1;
        } else if ( name_hash > note_hash ) {
            start = midle + 1;
        } else {
            
            ProfileBlockNamed( app, "solve collisions", solve_collisions );
            
            while ( midle - 1 >= start && hashes_notes->hashes[ midle - 1 ] == name_hash ) {
                midle--;
            }
            
            u8 current_weight = 0;
            
            while ( midle <= end && hashes_notes->hashes[ midle ] == name_hash ) {
                
                Code_Index_Note* note = hashes_notes->notes[ midle ];
                // Assert( note->kind < ArrayCount( type_weight ) );
                
                if ( type_weight[ note->note_kind ] > current_weight ) {
                    
                    if ( string_compare( name, note->text ) == 0 ) {
                        
                        current_weight = type_weight[ note->note_kind ];
                        result = note;
                        
                        if ( current_weight == 4 ) {
                            break;
                        }
                    }
                }
                
                midle++;
            }
            
            ProfileCloseNow( solve_collisions );
            
            break;
        }
    }
    
    return result;
}

/* NOTE: This funciton is a modification of 'draw_cpp_token_colors' from '4coder_draw.cpp'. */
function void primitive_highlight_draw_cpp_token_colors( Application_Links *app, Text_Layout_ID text_layout_id, Token_Array *array, Buffer_ID buffer ) {
    
    Range_i64 visible_range = text_layout_get_visible_range( app, text_layout_id );
    i64 first_index = token_index_from_pos( array, visible_range.first );
    Token_Iterator_Array it = token_iterator_index( 0, array, first_index );
    
    /* NOTE: Start modification. */
    Scratch_Block scratch( app );
    
#if 1
    code_index_lock( );
#endif
    
    Temp_Memory notes_temp = begin_temp( scratch );
    primitive_highlight_hashes_notes_t hashes_notes = primitive_highlight_create_big_note_array( app, scratch );
    
    ProfileBlockNamed( app, "token loop", token_loop );
    /* NOTE: End modification. */
    for ( ; ; ) {
        
        Token *token = token_it_read( &it );
        
        if ( token->pos >= visible_range.one_past_last ){
            break;
        }
        
        /* NOTE: Start modification. */
        FColor color = fcolor_id( defcolor_text_default );
        b32 colored = false;
        
        if ( token->kind == TokenBaseKind_Identifier ) {
            
            Temp_Memory temp = begin_temp( scratch );
            String_Const_u8 lexeme = push_token_lexeme( app, scratch, buffer, token );
            Code_Index_Note* note = primitive_highlight_get_note( app, &hashes_notes, lexeme );
            end_temp( temp );
            
            if ( note ) {
                
                switch ( note->note_kind ) {
                    case CodeIndexNote_Type: {
                        color = fcolor_id( primitive_highlight_type );
                    } break;
                    
                    case CodeIndexNote_Function: {
                        color = fcolor_id( primitive_highlight_function );
                    } break;
                    
                    case CodeIndexNote_Macro: {
                        color = fcolor_id( primitive_highlight_macro );
                    } break;
                    
                    case CodeIndexNote_4coderCommand: {
                        /* NOTE: 4coder doesn't create those notes as of 4.1.6. */
                        color = fcolor_id( primitive_highlight_4coder_command );
                    } break;
                }
                
                colored = true;
                
#if 1
                if ( note->note_kind == CodeIndexNote_Type ) {
                    
                    Token_Iterator_Array dot_arrow_it = it;
                    
                    if ( token_it_dec_non_whitespace( &dot_arrow_it ) ) {
                        
                        Token* dot_arrow = token_it_read( &dot_arrow_it );
                        
                        if ( dot_arrow->kind == TokenBaseKind_Operator && ( dot_arrow->sub_kind == TokenCppKind_Dot || dot_arrow->sub_kind == TokenCppKind_Arrow ) ) {
                            colored = false;
                        }
                    }
                }
#endif
            }
        }
        
        if( !colored ) {
            color = get_token_color_cpp( *token );
        }
        /* NOTE: End modification. */
        
        ARGB_Color argb = fcolor_resolve( color );
        paint_text_color( app, text_layout_id, Ii64_size( token->pos, token->size ), argb );
        
        if ( !token_it_inc_all( &it ) ){
            break;
        }
    }
    
#if 1
    code_index_unlock( );
#endif
    
    /* NOTE: Start modification. */
    ProfileCloseNow( token_loop );
    end_temp( notes_temp );
    /* NOTE: End modification. */
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
#endif


#if VERTICAL_BARS_SWITCH
function vertical_scope_annotation_start_t vertical_scope_annotation_get_start( Token_Array* tokens, i64 position, u32 flags ) {
    
    /* NOTE: The annotation_start_t struct iterator will point to the token preceding the one to use for the string. */
    vertical_scope_annotation_start_t annotation = { 0 };
    annotation.it = token_iterator_pos( 0, tokens, position );
    i64 parenthesis = 0;
    
    b32 contains_keyword = false;
    b32 contains_identifier = false;
    b32 contains_comment = false;
    b32 stop = false;
    
    while ( !stop && token_it_dec_non_whitespace( &annotation.it ) ) {
        
        Token *token = token_it_read( &annotation.it );
        
        if ( token->kind == TokenBaseKind_ParentheticalClose ) {
            parenthesis++;
        } else if ( token->kind == TokenBaseKind_ParentheticalOpen ) {
            parenthesis--;
        } else if ( parenthesis == 0 ) {
            
            switch ( token->kind ) {
                
                case TokenBaseKind_Identifier: {
                    
                    if ( token->flags == TokenBaseFlag_PreprocessorBody ) {
                        stop = true;
                    } else {
                        contains_identifier = true;
                    }
                } break;
                
                case TokenBaseKind_Preprocessor:
                case TokenBaseKind_LiteralInteger:
                case TokenBaseKind_LiteralFloat:
                case TokenBaseKind_LiteralString:
                case TokenBaseKind_ScopeOpen:
                case TokenBaseKind_ScopeClose: {
                    stop = true;
                } break;
                
                case TokenBaseKind_StatementClose: {
                    if ( token->sub_kind != TokenCppKind_Colon ) {
                        stop = true;
                    }
                } break;
                
                case TokenBaseKind_Comment: {
                    
                    if ( !( flags & vertical_scope_annotation_flag_no_comment ) ) {
                        
                        if ( !contains_keyword ) {
                            token_it_dec_non_whitespace( &annotation.it );
                            contains_comment = true;
                        };
                        stop = true;
                    }
                } break;
                
                case TokenBaseKind_Keyword: {
                    
                    if ( token->sub_kind == TokenCppKind_Else ) {
                        annotation.contains_else = true;
                    } else if ( token->sub_kind == TokenCppKind_If ) {
                        annotation.contains_if = true;
                    } else if ( token->sub_kind == TokenCppKind_Case && ( flags & vertical_scope_annotation_flag_only_last_case ) ) {
                        token_it_dec_non_whitespace( &annotation.it );
                        stop = true;
                    }
                    
                    contains_keyword = true;
                    
                } break;
            }
        }
    }
    
    annotation.is_valid = contains_keyword || contains_identifier || contains_comment;
    
    return annotation;
}

function i64 vertical_scope_annotation_get_text( Application_Links* app, Buffer_ID buffer, Token_Array* tokens, i64 position, u32 flags, String_u8* out, Range_i64* highlight_range ) {
    
    ProfileScope( app, "vertical scope annotation get_text" );
    
    Scratch_Block scratch( app );
    
    i64 first_character_position = 0;
    highlight_range->min = highlight_range->max = -1;
    
    vertical_scope_annotation_start_t annotation = vertical_scope_annotation_get_start( tokens, position, flags );
    b32 stop_after_new_line = false;
    b32 contains_else_of = false;
    
    if ( annotation.is_valid && annotation.contains_else && !annotation.contains_if ) {
        
        /* NOTE: if we are on "else", not "else if" we need to seek the if to get the condition. */
        annotation.is_valid = false;
        string_append( out, string_u8_litexpr( "else of " ) );
        contains_else_of = true;
        
        Token* token = token_it_read( &annotation.it );
        
        if ( token->kind == TokenBaseKind_ScopeClose ) {
            
            i64 if_scope_open = -1;
            b32 found = find_nest_side( app, buffer, token->pos - 1, FindNest_Balanced | FindNest_Scope, Scan_Backward, NestDelim_Open, &if_scope_open );
            
            if ( found ) {
                annotation = vertical_scope_annotation_get_start( tokens, if_scope_open, flags );
            }
            
        } else {
            
            /* NOTE: "if" without scope bracket, search backward for the first "if". */
            stop_after_new_line = true;
            
            while ( token_it_dec_non_whitespace( &annotation.it ) ) {
                token = token_it_read( &annotation.it );
                if ( token->kind == TokenBaseKind_Keyword && token->sub_kind == TokenCppKind_If ) {
                    /* NOTE: There could be a potential "else if" here so we need to call get_annotation_start again. */
                    annotation = vertical_scope_annotation_get_start( tokens, token->pos + token->size, flags );
                    break;
                }
            }
        }
    }
    
    if ( annotation.is_valid ) {
        
        Assert( !annotation.contains_else || annotation.contains_if );
        
        b32 has_token = token_it_inc_non_whitespace( &annotation.it );
        b32 stop = false;
        i64 parenthesis = 0;
        first_character_position = annotation.it.ptr->pos;
        b32 find_highlight = true;
        
        while ( !stop && has_token ) {
            
            Token* token = token_it_read( &annotation.it );
            
            if ( token->kind == TokenBaseKind_ParentheticalOpen ) {
                parenthesis++;
            } else if ( token->kind == TokenBaseKind_ParentheticalClose ) {
                parenthesis--;
                {
                    
                }} else if ( parenthesis == 0 ) {
                
                switch ( token->kind ) {
                    
                    case TokenBaseKind_ScopeOpen: {
                        stop = true;
                    } break;
                    
                    case TokenBaseKind_Whitespace: {
                        
                        /* NOTE: "if" without scope bracket. */
                        if ( stop_after_new_line ) {
                            u8 c = buffer_get_char( app, buffer, token->pos );
                            
                            if ( c == '\n' || c == '\r' ) {
                                stop = true;
                            }
                        }
                        
                    } break;
                }
            }
            
            if ( !stop ) {
                
                /* NOTE: a single whitespace token groups concecutive whitespaces and we want to only print one white space and no new lines.*/
                if ( token->kind == TokenBaseKind_Whitespace ) {
                    string_append( out, string_u8_litexpr( " " ) );
                } else {
                    
                    if ( token->kind != TokenBaseKind_Comment || !( flags & vertical_scope_annotation_flag_no_comment ) ) {
                        
                        /* NOTE: Should I highlight "else" in "else if" ? */
                        if ( find_highlight && ( token->kind == TokenBaseKind_Keyword || token->kind == TokenBaseKind_Identifier ) ) {
                            highlight_range->min = out->size;
                            highlight_range->max = out->size + token->size;
                        }
                        
                        b32 case_condition = false;
                        
                        if ( flags & vertical_scope_annotation_flag_highlight_case_label ) {
                            case_condition = ( token->kind == TokenBaseKind_StatementClose && token->sub_kind == TokenCppKind_Colon );
                        } else {
                            case_condition = ( token->kind == TokenBaseKind_Keyword && token->sub_kind == TokenCppKind_Case );
                        }
                        
                        if ( token->kind == TokenBaseKind_ParentheticalOpen || case_condition ) {
                            find_highlight = false;
                        }
                        
                        String_Const_u8 temp = push_token_lexeme( app, scratch, buffer, token );
                        string_append( out, temp );
                    }
                }
                
                has_token = token_it_inc_all( &annotation.it );
            }
        }
        
        if ( find_highlight ) {
            highlight_range->min = highlight_range->max = -1;
        } else if ( contains_else_of ) {
            highlight_range->min =  0;
        }
    }
    
    return first_character_position;
}

function void vertical_scope_annotation_draw( Application_Links *app, View_ID view, Buffer_ID buffer, Text_Layout_ID text_layout_id, u32 flags ) {
    
    ProfileScope( app, "vertical scope annotation" );
    
    Range_i64 visible_range = text_layout_get_visible_range( app, text_layout_id );
    Token_Array tokens = get_token_array_from_buffer( app, buffer );
    
    Face_ID face_id = get_face_id( app, buffer );
    Face_Metrics metrics = get_face_metrics( app, face_id );
    Rect_f32 region = text_layout_region( app, text_layout_id );
    
    Buffer_Scroll scroll = view_get_buffer_scroll( app, view );
    float x_start = region.x0 - scroll.position.pixel_shift.x;
    
    i64 indent_space_count = global_config.indent_width;
    
    if ( global_config.enable_virtual_whitespace ) {
        indent_space_count = global_config.virtual_whitespace_regular_indent;
    }
    
    Scratch_Block scratch( app );
    
    /* NOTE: Get the end of the textual line. */
    i64 new_pos = get_line_side_pos_from_pos( app, buffer, visible_range.min, Side_Max );
    Buffer_Cursor cursor = view_compute_cursor( app, view, seek_pos( new_pos ) );
    Vec2_f32 p = view_relative_xy_of_pos( app, view, cursor.line, cursor.pos );
    i64 end_of_line = view_pos_at_relative_xy( app, view, cursor.line, p );
    
    ProfileBlockNamed( app, "vertical scope annotation get_enclosure_ranges", get_enclosure_ranges_block );
    Range_i64_Array ranges = get_enclosure_ranges( app, scratch, buffer, end_of_line, RangeHighlightKind_CharacterHighlight );
    ProfileCloseNow( get_enclosure_ranges_block );
    
    ProfileBlockNamed( app, "vertical scope annotation ranges loop", ranges_loop );
    
    for ( i32 i = ranges.count - 1; i >= 0; i-- ) {
        
        Range_i64 range = ranges.ranges[ i ];
        
        u8 string_buffer[ 1024 ] = { 0 };
        String_u8 annotation = Su8( string_buffer, 0, ArrayCount( string_buffer ) );
        Range_i64 highlight = { -1, -1 };
        i64 first_character_position = vertical_scope_annotation_get_text( app, buffer, &tokens, range.min, flags, &annotation, &highlight );
        
        b32 draw = false;
        f32 region_top_offset = 0;
        
        if ( first_character_position >= visible_range.min ) {
            /* NOTE: Is the text only partially visible vertically ? */
            Rect_f32 rect = text_layout_character_on_screen( app, text_layout_id, first_character_position );
            
            if ( rect.y1 < region.y0 + metrics.text_height ) {
                rect = text_layout_character_on_screen( app, text_layout_id, range.min );
                region_top_offset = rect.y1 - region.y0;
                draw = true;
            }
        } else {
            
            /* NOTE: If an "if else" has the "else" in view but the "if" isn't, we need to adjust the region to avoid overlapping annotation with code. */
            if ( range.min >= visible_range.min ) {
                Rect_f32 rect = text_layout_character_on_screen( app, text_layout_id, range.min );
                region_top_offset = rect.y1 - region.y0;
            }
            
            draw = true;
        }
        
        if ( draw ) {
            
            ProfileScope( app, "vertical scope annotation draw" );
            
            b32 override_next_range = false;
            Range_i64 override_range = { 0 };
            
            if ( highlight.min > -1 ) {
                
                ProfileScope( app, "vertical scope annotation find else range of if" );
                
                /* NOTE simon: When there is an "if else" and the if is outside the view we want show the annotation for the else even if it's in view. */
                u64 keyword_size = annotation.size - highlight.min;
                b32 is_if = false;
                
                if ( keyword_size >= 2 ) {
                    is_if = ( annotation.str[ highlight.min ] == 'i' && annotation.str[ highlight.min + 1 ] == 'f' );
                    
                    if ( keyword_size > 2 ) {
                        is_if = is_if && ( annotation.str[ highlight.min + 2 ] == ' ' || annotation.str[ highlight.min + 2 ] == '(' );
                    }
                }
                
                if ( is_if ) {
                    
                    Token_Iterator_Array it = token_iterator_pos( 0, &tokens, range.max );
                    token_it_inc_non_whitespace( &it );
                    Token* token = token_it_read( &it );
                    
                    if ( token->kind == TokenBaseKind_Keyword && token->sub_kind == TokenCppKind_Else ) {
                        
                        token_it_inc_non_whitespace( &it );
                        token = token_it_read( &it );
                        
                        if ( token->kind == TokenBaseKind_ScopeOpen ) {
                            override_range.start = token->pos;
                            override_next_range = find_nest_side( app, buffer, token->pos + 1, FindNest_Scope | FindNest_Balanced | FindNest_EndOfToken, Scan_Forward, NestDelim_Close, &override_range.end );
                            if ( override_range.start >= visible_range.end || override_range.end <= visible_range.start ) {
                                override_next_range = false;
                            }
                        }
                    }
                }
            }
            
            float y0 = region.y0 + region_top_offset;
            float y1 = region.y1;
            
            if ( range.max - 1 < visible_range.max ) {
                ProfileScope( app, "vertical scope annotation text_layout_character_on_screen" );
                Rect_f32 rect = text_layout_character_on_screen( app, text_layout_id, range.end );
                y1 = rect.y0;
            }
            
            f32 max_size = y1 - y0;
            
            if ( max_size < 0 ) {
                max_size = 0;
            }
            
            ProfileBlockNamed( app, "vertical scope annotation get_string_advance", get_string_advance_block );
            
            f32 size = get_string_advance( app, face_id, SCu8( annotation.str, annotation.size ) );
#if 0
            while ( size > max_size && annotation.size > 1 ) {
                annotation.size -= 1;
                size = get_string_advance( app, face_id, SCu8( annotation.str, annotation.size ) );
            }
#else
            if ( size > max_size ) {
                
                f32 percentage = max_size / size;
                u64 string_size = ( u64 ) ( ( f32 ) annotation.size * percentage );
                
                size = get_string_advance( app, face_id, SCu8( annotation.str, string_size ) );
                
                if ( size > max_size ) {
                    while ( size > max_size && string_size > 1 ) {
                        string_size--;
                        size = get_string_advance( app, face_id, SCu8( annotation.str, string_size ) );
                    }
                } else {
                    f32 previous_size = size;
                    while ( size < max_size && string_size < annotation.size ) {
                        string_size++;
                        previous_size = size;
                        size = get_string_advance( app, face_id, SCu8( annotation.str, string_size ) );
                    }
                    
                    size = previous_size;
                    string_size--;
                }
                
                annotation.size = string_size;
            }
#endif
            ProfileCloseNow( get_string_advance_block );
            
            if ( size <= max_size ) {
                
                ProfileBlockNamed( app, "vertical scope annotation draw preparation", draw_prep );
                
                if ( highlight.min > ( i64 ) annotation.size - 1 ) {
                    highlight.min = -1;
                }
                
                if ( highlight.max > ( i64 ) annotation.size ) {
                    highlight.max = annotation.size;
                }
                
                Vec2_f32 position = { 0, 0 };
                Vec2_f32 delta = { 0, 0 };
                
                u32 orientation = flags & vertical_scope_annotation_orientation_mask;
                
                if ( orientation == vertical_scope_annotation_flag_top_to_bottom ) {
                    
                    float y = y0 + ( max_size - size ) * 0.5f;
                    float x_centering = ( ( indent_space_count * metrics.space_advance ) - metrics.line_height ) * 0.5f;
                    i64 space_count = indent_space_count * ( ranges.count - 1 - i );
                    float x = x_start + space_count * metrics.space_advance + metrics.line_height + x_centering;
                    position = { x, y };
                    delta = V2f32( 0.0f, 1.0f );
                    
                } else {
                    
                    float y = y1 - ( max_size - size ) * 0.5f;
                    float x_centering = ( ( indent_space_count * metrics.space_advance ) - metrics.line_height ) * 0.5f;
                    i64 space_count = indent_space_count * ( ranges.count - 1 - i );
                    float x = x_start + space_count * metrics.space_advance + x_centering;
                    position = { x, y };
                    delta = V2f32( 0.0f, -1.0f );
                }
                
                String_Const_u8 str_1, str_2, str_3;
                
                if ( highlight.min >= 0 && ( flags & vertical_scope_annotation_flag_highlight ) ) {
                    str_1 = SCu8( annotation.str, highlight.min );
                    str_2 = SCu8( str_1.str + str_1.size, highlight.max - highlight.min );
                    str_3 = SCu8( str_2.str + str_2.size, annotation.size - ( str_1.size + str_2.size ) );
                } else {
                    str_1 = SCu8( annotation.str, annotation.size );
                    str_2 = str_3 = SCu8( ( u8* ) 0, ( u64 ) 0 );
                }
                
                ProfileCloseNow( draw_prep );
                
                ProfileBlockNamed( app, "vertical scope annotation draw strings", draw_strings );                    
                
                draw_string_oriented( app, face_id, finalize_color( vertical_scope_annotation_text, 0 ), str_1, position, 0, delta );
                
                if ( str_2.size ) {
                    f32 str_size = get_string_advance( app, face_id, str_1 );
                    position.y += delta.y * str_size;
                    draw_string_oriented( app, face_id, finalize_color( vertical_scope_annotation_highlight, 0 ), str_2, position, 0, delta );
                }
                
                if ( str_3.size ) {
                    f32 str_size = get_string_advance( app, face_id, str_2 );
                    position.y += delta.y * str_size;
                    draw_string_oriented( app, face_id, finalize_color( vertical_scope_annotation_text, 0 ), str_3, position, 0, delta );
                }
                
                ProfileCloseNow( draw_strings );
            }
            
            if ( override_next_range ) {
                ranges.ranges[ i ] = override_range;
                i++;
            }
        }
    }
    
    ProfileCloseNow( ranges_loop );
}

#endif

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
    
#if VERTICAL_BARS_SWITCH
    vertical_scope_annotation_draw(app, view_id, buffer, text_layout_id, vertical_scope_annotation_flag_bottom_to_top |
                                   vertical_scope_annotation_flag_highlight);
#endif
    
    // NOTE(allen): draw line numbers
    if (global_config.show_line_number_margins){
        draw_line_number_margin(app, view_id, buffer, face_id, text_layout_id, line_number_rect);
    }
    
    // NOTE(allen): draw the buffer
#if VERTICAL_BARS_SWITCH
    krz_render_buffer(app, view_id, face_id, buffer, text_layout_id, region);
#else
    default_render_buffer(app, view_id, face_id, buffer, text_layout_id, region);
#endif
    
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

CUSTOM_COMMAND_SIG(case_sensitive_mode_toggle)
{
    case_sensitive_mode = !case_sensitive_mode;
}

CUSTOM_COMMAND_SIG(put_new_line_bellow)
{
    //push_buffer_line(Application_Links *app, Arena *arena, Buffer_ID buffer, i64 line_number){
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    i64 pos = view_get_cursor_pos(app, view);
    i64 line = get_line_number_from_pos(app, buffer, pos);
    line += 1;
    Scratch_Block scratch(app);
    String_Const_u8 s = push_buffer_line(app, scratch, buffer, line);
    s = push_u8_stringf(scratch, "\n");
    pos = get_line_side_pos(app, buffer, line, Side_Min);
    buffer_replace_range(app, buffer, Ii64(pos), s);
    view_set_cursor_and_preferred_x(app, view, seek_line_col(line, 0));
}

// --------- Mouse fix (for bottom bar) and select on click --------- //
// fix: add one line to line_number
function i64
krz_fix_view_pos_from_xy(Application_Links *app, View_ID view, Vec2_f32 p){
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    Rect_f32 region = view_get_buffer_region(app, view);
    f32 width = rect_width(region);
    Face_ID face_id = get_face_id(app, buffer);
    Buffer_Scroll scroll_vars = view_get_buffer_scroll(app, view);
    i64 line = scroll_vars.position.line_number + 1;
    p = (p - region.p0) + scroll_vars.position.pixel_shift;
    return(buffer_pos_at_relative_xy(app, buffer, width, face_id, line, p));
}

global bool is_selected;
CUSTOM_COMMAND_SIG(krz_click_set_cursor_and_mark)
CUSTOM_DOC("Sets the cursor position and mark to the mouse position.")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Mouse_State mouse = get_mouse_state(app);
    i64 pos = krz_fix_view_pos_from_xy(app, view, V2f32(mouse.p));
    
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    
    // NOTE: Select on click yeee
    Range_i64 range = enclose_pos_alpha_numeric_underscore(app, buffer, pos);
    
    select_scope(app, view, range);
    is_selected = true;
}

CUSTOM_COMMAND_SIG(krz_click_set_cursor_if_lbutton)
CUSTOM_DOC("If the mouse left button is pressed, sets the cursor position to the mouse position.")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Mouse_State mouse = get_mouse_state(app);
    if (mouse.l){
        i64 pos = krz_fix_view_pos_from_xy(app, view, V2f32(mouse.p));
        view_set_cursor_and_preferred_x(app, view, seek_pos(pos));
        
        // NOTE: Select on click yeee
        if(is_selected)
        {
            view_set_mark(app, view, seek_pos(pos));
            is_selected = false;
        }
    }
    no_mark_snap_to_cursor(app, view);
    set_next_rewrite(app, view, Rewrite_NoChange);
}

// --------- --------- //

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

enum enum_comment
{
    NO_COMMENT,
    COMMENT_WITH_SPACE,
    COMMENT_WITHOUT_SPACE,
};

function enum_comment
krz_c_line_comment_starts_at_position(Application_Links *app, Buffer_ID buffer, i64 pos){
    enum_comment alread_has_comment = NO_COMMENT;
    u8 check_buffer[3];
    
    if (buffer_read_range(app, buffer, Ii64(pos, pos + 3), check_buffer)){
        if (check_buffer[0] == '/' && check_buffer[1] == '/' && check_buffer[2] == ' '){
            alread_has_comment = COMMENT_WITH_SPACE;
        }
        else if (check_buffer[0] == '/' && check_buffer[1] == '/'){
            alread_has_comment = COMMENT_WITHOUT_SPACE;
        }
    }
    return(alread_has_comment);
}

CUSTOM_COMMAND_SIG(krz_comment_lines_toggle)
CUSTOM_DOC("Turns uncommented lines into commented lines and vice versa for comments starting with '//'.")
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    History_Group group = history_group_begin(app, buffer);
    
    selected_lines_info selection = get_selected_lines_info(app, view, buffer);
    
    i64 number_of_lines = selection.max_line - selection.min_line + 1;
    
    for(i64 i = 0; i < number_of_lines; i++){
        i64 current_line = selection.min_line + i;
        
        i64 pos = get_line_start_pos(app, buffer, current_line);
        
#if 1
        b32 comment = c_line_comment_starts_at_position(app, buffer, pos);
        if(comment)
        {
            buffer_replace_range(app, buffer, Ii64(pos, pos + 2), string_u8_empty);
        }
        else
        {
            buffer_replace_range(app, buffer, Ii64(pos), string_u8_litexpr("//"));
        }
#else
        enum_comment already_has_comment = krz_c_line_comment_starts_at_position(app, buffer, pos);
        if (already_has_comment == COMMENT_WITHOUT_SPACE){
            buffer_replace_range(app, buffer, Ii64(pos, pos + 2), string_u8_empty);
        }
        else if(already_has_comment == COMMENT_WITH_SPACE){
            buffer_replace_range(app, buffer, Ii64(pos, pos + 3), string_u8_empty);
        }
        else{
            buffer_replace_range(app, buffer, Ii64(pos), string_u8_litexpr("// "));
        }
#endif
    }
    
    history_group_end(group);
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
    
    mapping_init(tctx, &framework_mapping);
#if OS_MAC
    setup_mac_mapping(&framework_mapping, mapid_global, mapid_file, mapid_code);
#else
    MappingScope();
    
    
    // -------------------------------------------------------------- //
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
    
    Bind(krz_project_fkey_command, KeyCode_F5);
    Bind(krz_project_fkey_command, KeyCode_F6);
    Bind(krz_project_fkey_command, KeyCode_F7);
    Bind(krz_project_fkey_command, KeyCode_F8);
    
    Bind(split_fullscreen_mode, KeyCode_F12);
    
    Bind(load_theme_current_buffer, KeyCode_T, KeyCode_Control, KeyCode_Shift, KeyCode_Alt);
    
    BindMouseWheel(mouse_wheel_scroll);
    BindMouseWheel(mouse_wheel_change_face_size, KeyCode_Control);
    
    // -------------------------------------------------------------- //
    SelectMap(mapid_file);
    ParentMap(mapid_global);
    
    BindTextInput(write_text_input);
    
    Bind(set_jump_table_1, KeyCode_F1, KeyCode_Shift);
    Bind(goto_jump_table_1, KeyCode_F1);
    
    Bind(set_jump_table_2, KeyCode_F2, KeyCode_Shift);
    Bind(goto_jump_table_2, KeyCode_F2);
    
    Bind(set_jump_table_3, KeyCode_F3, KeyCode_Shift);
    Bind(goto_jump_table_3, KeyCode_F3);
    
    Bind(set_jump_table_4, KeyCode_F4, KeyCode_Shift);
    Bind(goto_jump_table_4, KeyCode_F4);
    
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
    
    // -------------------------------------------------------------- //
    
    SelectMap(mapid_code);
    ParentMap(mapid_file);
    BindTextInput(write_text_and_auto_indent);
    
    // WORD COMPLETE
    Bind(word_complete,              KeyCode_Tab);
    Bind(word_complete_drop_down,    KeyCode_Space, KeyCode_Control);
    
    
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
    Bind(write_comment_section, KeyCode_BackwardSlash, KeyCode_Control);
#endif
}

#endif //FCODER_DEFAULT_BINDINGS

// BOTTOM
