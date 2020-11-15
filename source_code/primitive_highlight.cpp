

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