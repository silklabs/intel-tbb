/*
    Copyright 2005-2013 Intel Corporation.  All Rights Reserved.

    This file is part of Threading Building Blocks.

    Threading Building Blocks is free software; you can redistribute it
    and/or modify it under the terms of the GNU General Public License
    version 2 as published by the Free Software Foundation.

    Threading Building Blocks is distributed in the hope that it will be
    useful, but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Threading Building Blocks; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    As a special exception, you may use this file as part of a free software
    library without restriction.  Specifically, if other files instantiate
    templates or use macros or inline functions from this file, or you compile
    this file and link it with other files to produce an executable, this
    file does not by itself cause the resulting executable to be covered by
    the GNU General Public License.  This exception does not however
    invalidate any other reasons why the executable file might be covered by
    the GNU General Public License.
*/

#import "OpenGLView.h"
#import <OpenGL/gl.h>
#import "tbbAppDelegate.h"

int x,y;
void on_mouse_func(int x, int y, int k);
void on_key_func(int x);
void* windows_ptr=0;

//defined in cpp-file
extern char* window_title;
extern int cocoa_update;
extern int g_sizex, g_sizey;
extern unsigned int *g_pImg;

void draw(void)
{
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_POINT);
    glDrawPixels(g_sizex, g_sizey, GL_BGRA_EXT, GL_UNSIGNED_INT_8_8_8_8_REV, g_pImg);
    glEnd();
    glFlush();
}

@implementation OpenGLView

@synthesize timer;

- (void) drawRect:(NSRect)start
{
    x=g_sizex;
    y=g_sizey;
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    draw();
    glFlush();
    timer = [NSTimer scheduledTimerWithTimeInterval:0.03 target:self selector:@selector(update_window) userInfo:nil repeats:YES];
}

-(void) update_window{
    if( cocoa_update ) draw();
    if( window_title )[_window setTitle:[NSString stringWithFormat:@"%s", window_title]];
    return;
}

-(void) keyDown:(NSEvent *)theEvent{
    on_key_func([theEvent.characters characterAtIndex:0]);
    return;
}

-(void) mouseDown:(NSEvent *)theEvent{
    // mouse event for seismic and fractal
    NSPoint point= theEvent.locationInWindow;
    x = point.x;
    y = point.y;
    NSRect rect = self.visibleRect;
    on_mouse_func(x*g_sizex/rect.size.width,y*g_sizey/rect.size.height,1);
    draw();
    return;
    
}

- (BOOL) acceptsFirstResponder
{
    return YES;
}

- (void) rightMouseDown:(NSEvent *)theEvent
{
    return;
}

-(void) viewDidEndLiveResize
{
    NSRect rect = self.visibleRect;
    x=rect.size.width;
    y=rect.size.height;
    glPixelZoom((float)x/(float)g_sizex, (float)y/(float)g_sizey);
    [_window setTitle:[NSString stringWithFormat:@"X=%d Y=%d", x,y]];
    draw();
    return;
}

@end
