struct selected_lines_info
{
    i64 cursor_pos;
    i64 mark_pos;
    
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
    
    result.cursor_pos = view_get_cursor_pos(app, view);
	result.mark_pos = view_get_mark_pos(app, view);
    
	result.min_pos = Min(result.cursor_pos, result.mark_pos);
	result.max_pos = Max(result.cursor_pos, result.mark_pos);
    
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

CUSTOM_COMMAND_SIG(move_lines_up)
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
    if(painter_mode) return;
    
    View_ID view = get_active_view(app, Access_ReadVisible);
    Mouse_State mouse = get_mouse_state(app);
#if BAR_POSITION_BOT
    i64 pos = krz_fix_view_pos_from_xy(app, view, V2f32(mouse.p));
#else
    i64 pos = view_pos_from_xy(app, view, V2f32(mouse.p));
#endif
    
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    
    // NOTE: Select on click yeee
    Range_i64 range = enclose_pos_alpha_numeric_underscore(app, buffer, pos);
    
    select_scope(app, view, range);
    is_selected = true;
}

CUSTOM_COMMAND_SIG(krz_click_set_cursor_if_lbutton)
CUSTOM_DOC("If the mouse left button is pressed, sets the cursor position to the mouse position.")
{
    if(painter_mode) return;
    
    View_ID view = get_active_view(app, Access_ReadVisible);
    Mouse_State mouse = get_mouse_state(app);
    if (mouse.l){
#if BAR_POSITION_BOT
        i64 pos = krz_fix_view_pos_from_xy(app, view, V2f32(mouse.p));
#else
        i64 pos = view_pos_from_xy(app, view, V2f32(mouse.p));
#endif
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

// --------- Mouse fix (for bottom bar) and select on click --------- //
// ------------------------------------------------------------------ //

CUSTOM_COMMAND_SIG(krz_comment_lines_toggle)
CUSTOM_DOC("Turns uncommented lines into commented lines and vice versa for comments starting with '//'.")
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    History_Group group = history_group_begin(app, buffer);
    
    selected_lines_info selection = get_selected_lines_info(app, view, buffer);
    // if there is a comment on selection start
    // we skip adding new comments and vice versa if there is no comment
    // b32 comment_on_start_selection = c_line_comment_starts_at_position(app, buffer, selection.cursor_pos);
    
    
    i64 number_of_lines = selection.max_line - selection.min_line + 1;
    
    for(i64 i = 0; i < number_of_lines; i++){
        i64 current_line = selection.min_line + i;
        i64 pos = get_pos_past_lead_whitespace_from_line_number(app, buffer, current_line);
        b32 comment = c_line_comment_starts_at_position(app, buffer, pos);
        if(comment)
        {
            buffer_replace_range(app, buffer, Ii64(pos, pos + 2), string_u8_empty);
        }
        else if(!comment)
        {
            buffer_replace_range(app, buffer, Ii64(pos), string_u8_litexpr("// "));
        }
    }
    
    history_group_end(group);
}

// 1 == left to right or 0 ==right to left 
function void
enclose_selection(Application_Links *app, String_Const_u8 left, String_Const_u8 right, b32 left_to_right){
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    selected_lines_info selection = get_selected_lines_info(app, view, buffer);
    History_Group group = history_group_begin(app, buffer);
    
    if(left_to_right){
        buffer_replace_range(app, buffer, Ii64(selection.min_pos), left);
        if(selection.cursor_pos != selection.mark_pos){
            buffer_replace_range(app, buffer, Ii64(selection.max_pos + right.size), right);
        }
        view_set_cursor_and_preferred_x(app, view, seek_pos(selection.cursor_pos + right.size));
    } else {
        buffer_replace_range(app, buffer, Ii64(selection.max_pos), right);
        if(selection.cursor_pos != selection.mark_pos){
            buffer_replace_range(app, buffer, Ii64(selection.min_pos), left);
        }
        view_set_cursor_and_preferred_x(app, view, seek_pos(selection.cursor_pos + 1));
    }
    
    history_group_end(group);
}

CUSTOM_COMMAND_SIG(write_paren_on_selection_left)
CUSTOM_DOC("Encloses the selection with parenthesis")
{
    enclose_selection(app, string_u8_litexpr("("), string_u8_litexpr(")"), true);
}

CUSTOM_COMMAND_SIG(write_paren_on_selection_right)
CUSTOM_DOC("Encloses the selection with parenthesis")
{
    enclose_selection(app, string_u8_litexpr("("), string_u8_litexpr(")"), false);
}

CUSTOM_COMMAND_SIG(write_curly_brace_on_selection_left)
CUSTOM_DOC("Encloses the selection with curly_brace")
{
    enclose_selection(app, string_u8_litexpr("{"), string_u8_litexpr("}"), true);
}

CUSTOM_COMMAND_SIG(write_curly_brace_on_selection_right)
CUSTOM_DOC("Encloses the selection with curly_brace")
{
    enclose_selection(app, string_u8_litexpr("{"), string_u8_litexpr("}"), false);
}

CUSTOM_COMMAND_SIG(write_bracket_on_selection_left)
CUSTOM_DOC("Encloses the selection with bracket")
{
    enclose_selection(app, string_u8_litexpr("["), string_u8_litexpr("]"), true);
}

CUSTOM_COMMAND_SIG(write_bracket_on_selection_right)
CUSTOM_DOC("Encloses the selection with bracket")
{
    enclose_selection(app, string_u8_litexpr("["), string_u8_litexpr("]"), false);
}

CUSTOM_COMMAND_SIG(write_quote_on_selection_right)
CUSTOM_DOC("Encloses the selection with quote")
{
    enclose_selection(app, string_u8_litexpr("["), string_u8_litexpr("]"), false);
}

CUSTOM_COMMAND_SIG(write_quote_on_selection)
CUSTOM_DOC("Encloses the selection with quote")
{
    enclose_selection(app, string_u8_litexpr("\""), string_u8_litexpr("\""), true);
}

CUSTOM_COMMAND_SIG(write_single_quote_on_selection)
CUSTOM_DOC("Encloses the selection with single_quote")
{
    enclose_selection(app, string_u8_litexpr("\'"), string_u8_litexpr("\'"), true);
}
