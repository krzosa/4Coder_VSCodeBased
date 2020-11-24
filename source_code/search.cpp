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
    
    selected_lines_info selection = get_selected_lines_info(app, 
                                                            view, buffer);
    
    i64 selection_size = selection.max_pos - selection.min_pos;
    i64 selection_lines = selection.max_line - selection.min_line;
    if (selection_size < max_selection_size && selection_lines == 0)
    {
        Scratch_Block scratch(app);
        
        Range_i64 range = {};
        range.start = selection.min_pos;
        range.end = selection.max_pos;
        
        String_Const_u8 query = push_buffer_range(app, scratch, 
                                                  buffer, range);
        
        krz_isearch_case_mode_aware(app, Scan_Forward, range.first, query);
    }
    else
    {
        krz_isearch_case_mode_aware(app, Scan_Forward, 
                                    selection.cursor_pos, SCu8());
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

CUSTOM_COMMAND_SIG(krz_query_replace_selection)
{
    View_ID view = get_active_view(app, Access_Always);
	Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    if(buffer == 0) return;
    
    selected_lines_info selection = get_selected_lines_info(app, 
                                                            view, buffer);
    
    i64 selection_size = selection.max_pos - selection.min_pos;
    i64 selection_lines = selection.max_line - selection.min_line;
    if (selection_size < max_selection_size && selection_lines == 0)
    {
        Scratch_Block scratch(app);
        
        Range_i64 range = {};
        range.start = selection.min_pos;
        range.end = selection.max_pos;
        
        String_Const_u8 replace = push_buffer_range(app, scratch, 
                                                    buffer, range);
        
        krz_query_replace_parameter(app, replace, range.min, true);
    }
    else
    {
        krz_query_replace_parameter(app, SCu8(), 
                                    selection.cursor_pos, true);
    }
    
    // TODO: Set cursor based on what was clicked 
    // escape come back to start of query else stay on current word
    view_set_cursor(app, view, seek_pos(selection.cursor_pos));
}

CUSTOM_COMMAND_SIG(krz_list_all_locations_of_selection)
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
        if(case_sensitive_mode) list_all_locations(app);
        else list_all_locations_case_insensitive(app);
    }
    else
    {
        if(case_sensitive_mode) list_all_locations_of_selection(app);
        else list_all_locations_of_selection_case_insensitive(app);
    }
}
