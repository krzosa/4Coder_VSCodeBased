struct brush_in_time
{
    Vec2_i32 p;
    b8 mouse_l;
};

#define max_size_of_array 10000
global b32 painter_mode = false;
global brush_in_time *brush_strokes;
global i64 brush_strokes_size;
global i32 brush_size = 20;
global i32 brush_size_control = 5;


function void
painter_whole_screen_render_caller(Application_Links *app, Frame_Info frame_info){
    if(!painter_mode) return;
    
    Rect_f32 region = global_get_screen_rectangle(app);
    Vec2_f32 center = rect_center(region);
    
    // Face_ID face_id = get_face_id(app, 0);
    Mouse_State mouse = get_mouse_state(app);
    Scratch_Block scratch(app);
    
    
    // Collect an array of points when mouse was clicked
    // collect the minimum of points when mouse was not clicked
    // to determine when to stop drawing
    static bool prev_mouse_state;
    if(mouse.l)
    {
        if(brush_strokes_size < max_size_of_array)
        {
            brush_strokes[brush_strokes_size].mouse_l = mouse.l;
            brush_strokes[brush_strokes_size++].p = mouse.p;
        }
    }
    else if(mouse.l == 0 && prev_mouse_state == 1)
    {
        if(brush_strokes_size < max_size_of_array)
        {
            brush_strokes[brush_strokes_size].mouse_l = mouse.l;
            brush_strokes[brush_strokes_size++].p = mouse.p;
        }
    }
    prev_mouse_state = mouse.l;
    
    if(brush_strokes_size > 1)
    {
        for(i32 j = 1; j < brush_strokes_size; j++)
        {
            // Special case for a single dot in the wild
            if(brush_strokes[j-1].mouse_l == 0 || brush_strokes[j].mouse_l == 1)
            {
                Rect_f32 rect = {(f32)brush_strokes[j].p.x - (brush_size / 2), 
                    (f32)brush_strokes[j].p.y - (brush_size / 2), 
                    (f32)brush_strokes[j].p.x + brush_size / 2, 
                    (f32)brush_strokes[j].p.y + brush_size / 2};
                draw_rectangle_fcolor(app, rect, 10.f, fcolor_id(defcolor_text_default));
            }
            // Skip if mouse not pressed
            if(brush_strokes[j-1].mouse_l == false || brush_strokes[j].mouse_l == false)
                continue;
            
            Vec2_i32 minP = brush_strokes[j-1].p;
            Vec2_i32 maxP = brush_strokes[j].p;
            
            // Drawing line algorithm, fill every pixel with a rectangle ! ;> 
            bool steep = false; 
            if (abs(minP.x-maxP.x) < abs(minP.y-maxP.y)) 
            {
                i32 temp = minP.x;
                minP.x = minP.y;
                minP.y = temp;
                
                temp = maxP.x;
                maxP.x = maxP.y;
                maxP.y = temp;
                steep = true; 
            } 
            
            if(minP.x > maxP.x)
            {
                Vec2_i32 temp = minP;
                minP = maxP;
                maxP = temp;
            }
            
            for(i32 x = minP.x; x < maxP.x; x++)
            {
                f32 t = (x - minP.x ) / (f32)(maxP.x - minP.x); 
                i32 y = (i32)(minP.y * (1. - t) + maxP.y*t); 
                Vec2_i32 pos;
                if(steep)
                {
                    pos = {y, x};
                }
                else 
                {
                    pos = {x, y};
                }
                
                Rect_f32 rect = {(f32)pos.x - (brush_size / 2), 
                    (f32)pos.y - (brush_size / 2), 
                    (f32)pos.x + brush_size / 2, 
                    (f32)pos.y + brush_size / 2};
                draw_rectangle_fcolor(app, rect, 10.f, fcolor_id(defcolor_text_default));
            }
            
        }
        
    }
}