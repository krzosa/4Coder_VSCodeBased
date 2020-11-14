// TODO
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
