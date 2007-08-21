//  $Id$
//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2004-2005 Steve Baker <sjbaker1@airmail.net>
//  Copyright (C) 2006 Joerg Henrichs, SuperTuxKart-Team, Steve Baker
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include <SDL/SDL.h>
#include "user_config.hpp"
#include "race_gui.hpp"
#include "history.hpp"
#include "widget_set.hpp"
#include "track.hpp"
#include "material_manager.hpp"
#include "menu_manager.hpp"
#include "sdldrv.hpp"
#include "translation.hpp"
#include "font.hpp"

RaceGUI::RaceGUI(): m_time_left(0.0)
{
    if(user_config->m_fullscreen)
    {
        SDL_ShowCursor(SDL_DISABLE);
    }

    if(!user_config->m_profile)
    {
        UpdateKeyboardMappings();
    }   // if !user_config->m_profile

    // FIXME: translation problem
    m_pos_string[0] = "?!?";
    m_pos_string[1] = "1st";
    m_pos_string[2] = "2nd";
    m_pos_string[3] = "3rd";
    m_pos_string[4] = "4th";
    m_pos_string[5] = "5th";
    m_pos_string[6] = "6th";
    m_pos_string[7] = "7th";
    m_pos_string[8] = "8th";
    m_pos_string[9] = "9th";
    m_pos_string[10] = "10th";

    //FIXME: Temporary, we need a better icon here
    m_steering_wheel_icon = material_manager->getMaterial("wheel.rgb");
    m_steering_wheel_icon->getState()->disable(GL_CULL_FACE);
    m_speed_back_icon = material_manager->getMaterial("speedback.rgb");
    m_speed_back_icon->getState()->disable(GL_CULL_FACE);
    m_speed_fore_icon = material_manager->getMaterial("speedfore.rgb");
    m_speed_fore_icon->getState()->disable(GL_CULL_FACE);

    m_fps_counter = 0;
    m_fps_string[0]=0;
    m_fps_timer.reset();
    m_fps_timer.update();
    m_fps_timer.setMaxDelta(1000);

}   // RaceGUI

//-----------------------------------------------------------------------------
RaceGUI::~RaceGUI()
{
    if(user_config->m_fullscreen)
    {
        SDL_ShowCursor(SDL_ENABLE);
    }
    //FIXME: does all that material stuff need freeing somehow?
}   // ~Racegui

//-----------------------------------------------------------------------------
void RaceGUI::UpdateKeyboardMappings()
{
    // Clear all entries.
    for(int type = 0;type< (int) IT_LAST+1;type++)
        for(int id0=0;id0<MAX_ID0;id0++)
            for(int id1=0;id1<MAX_ID1;id1++)
                for(int id2=0;id2<MAX_ID2;id2++)
                    m_input_map[type][id0][id1][id2].kart = NULL;


    // Defines the mappings for player keys to kart and action
    // To avoid looping over all players to find out what
    // player control key was pressed, a special data structure
    // is set up: keysToKArt contains for each (player assigned)
    // key which kart it applies to (and therefore which player),
    // and typeForKey contains the assigned function of that key.
    const int NUM = world->m_race_setup.getNumPlayers();
    for(int i=0; i < NUM; i++)
    {
        PlayerKart* kart = world->getPlayerKart(i);

        for(int ka=(int) KC_LEFT;ka< (int) KC_FIRE+1;ka++)
            putEntry(kart, (KartActions) ka);
    }

}   // UpdateKeyControl

//-----------------------------------------------------------------------------
void RaceGUI::putEntry(PlayerKart *kart, KartActions kc)
{
    Player *p = kart->getPlayer();
    const Input *I  = p->getInput(kc);

    m_input_map[I->type][I->id0][I->id1][I->id2].kart = kart;
    m_input_map[I->type][I->id0][I->id1][I->id2].action = kc;
}

//-----------------------------------------------------------------------------
bool RaceGUI::handleInput(InputType type, int id0, int id1, int id2, int value)
{
    PlayerKart *k = m_input_map[type][id0][id1][id2].kart;

    if (k)
    {
        k->action(m_input_map[type][id0][id1][id2].action, value);
        return true;
    }
    else
        return false;
}

//-----------------------------------------------------------------------------
void RaceGUI::update(float dt)
{
    assert(world != NULL);
    drawStatusText(world->m_race_setup, dt);
    cleanupMessages();
}   // update

//-----------------------------------------------------------------------------
void RaceGUI::input(InputType type, int id0, int id1, int id2, int value)
{
    switch (type)
    {
    case IT_KEYBOARD:
        // Stuff that handleInput() does not care about are
        // internal keyboard actions.
        if (!handleInput(type, id0, id1, id2, value))
            inputKeyboard(id0, value);
        break;
    default:  // no keyboard event
        handleInput(type, id0, id1, id2, value);
        break;
    }

}

//-----------------------------------------------------------------------------
void RaceGUI::inputKeyboard(int key, int pressed)
{
    if (!pressed)
        return;

    static int isWireframe = false ;
    switch ( key )
    {
    case SDLK_F7:
        if(world->m_race_setup.getNumPlayers()==1)
        {   // ctrl-r
            Kart* kart = world->getPlayerKart(0);
            kart->setCollectable((rand()%2)?COLLECT_MISSILE :COLLECT_HOMING_MISSILE, 10000);
        }
        break;
    case SDLK_F12:
        user_config->m_display_fps = !user_config->m_display_fps;
        if(user_config->m_display_fps)
        {
            m_fps_timer.reset();
            m_fps_timer.setMaxDelta(1000);
            m_fps_counter=0;
        }
        break;
#ifdef BULLET
    case SDLK_F2:
        user_config->m_bullet_debug = !user_config->m_bullet_debug;
        break;
#endif
    case SDLK_F11:
        glPolygonMode(GL_FRONT_AND_BACK, isWireframe ? GL_FILL : GL_LINE);
        isWireframe = ! isWireframe;
        break;
#ifndef WIN32
        // For now disable F9 toggling fullscreen, since windows requires
        // to reload all textures, display lists etc. Fullscreen can
        // be toggled from the main menu (options->display).
    case SDLK_F9:
        drv_toggleFullscreen(0);   // 0: do not reset textures
        // Fall through to put the game into pause mode.
#endif
    case SDLK_ESCAPE: // ESC
        world->pause();
        menu_manager->pushMenu(MENUID_RACEMENU);
        break;
    case SDLK_F10:
        history->Save();
        break;
    default:
        break;
    } // switch
} // inputKeyboard

//-----------------------------------------------------------------------------
void RaceGUI::drawFPS ()
{
    if (++m_fps_counter>=50)
    {
        m_fps_timer.update();
        sprintf(m_fps_string, "%d",
                (int)(m_fps_counter/m_fps_timer.getDeltaTime()));
        m_fps_counter = 0;
        m_fps_timer.setMaxDelta(1000);
    }
    font_race->PrintShadow(m_fps_string,48, 0, user_config->m_height-50);
}   // drawFPS

//-----------------------------------------------------------------------------
#if 0
//This is not being used..
void RaceGUI::drawTexture(const GLuint texture, int w, int h,
                          int red, int green, int blue, int x, int y)
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);

    glColor3ub ( red, green, blue ) ;
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex2f(x, (float)h+y);

    glTexCoord2f(1, 0);
    glVertex2f((float)w+x, (float)h+y);

    glTexCoord2f(1, 1);
    glVertex2f((float)w+x, y);

    glTexCoord2f(0, 1);
    glVertex2f(x, y);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}   // drawTexture
#endif
//-----------------------------------------------------------------------------
void RaceGUI::drawTimer ()
{
    if(world->getPhase()!=World::RACE_PHASE         &&
       world->getPhase()!=World::DELAY_FINISH_PHASE   ) return;
    char str[256];

    assert(world != NULL);
    m_time_left = world->m_clock;

    TimeToString(m_time_left, str);
    font_race->PrintShadow(str, 60, user_config->m_width-260, 
                           user_config->m_height-64);
}   // drawTimer

//-----------------------------------------------------------------------------
#define TRACKVIEW_SIZE 100

void RaceGUI::drawMap ()
{
    glDisable ( GL_TEXTURE_2D ) ;
    assert(world != NULL);
    int xLeft = 10;
    int yTop   =  10;

    world -> m_track -> draw2Dview ( xLeft,   yTop   );

    glBegin ( GL_QUADS ) ;

    for ( unsigned int i = 0 ; i < world->getNumKarts() ; i++ )
    {
        sgCoord *c ;

        Kart* kart = world->getKart(i);
        glColor3fv ( *kart->getColor());
        c          = kart->getCoord () ;

        /* If it's a player, draw a bigger sign */
        if (kart -> isPlayerKart ())
        {
            world -> m_track->glVtx ( c->xyz, xLeft+3, yTop+3);
            world -> m_track->glVtx ( c->xyz, xLeft-2, yTop+3);
            world -> m_track->glVtx ( c->xyz, xLeft-2, yTop-2);
            world -> m_track->glVtx ( c->xyz, xLeft+3, yTop-2);
            /*      world -> m_track->glVtx ( c->xyz, xLeft  , yTop-4);
                  world -> m_track->glVtx ( c->xyz, xLeft+4, yTop  );
                  world -> m_track->glVtx ( c->xyz, xLeft  , yTop+4);
                  world -> m_track->glVtx ( c->xyz, xLeft-4, yTop  ); */
        }
        else
        {
            world -> m_track->glVtx ( c->xyz, xLeft+2, yTop+2);
            world -> m_track->glVtx ( c->xyz, xLeft-1, yTop+2);
            world -> m_track->glVtx ( c->xyz, xLeft-1, yTop-1);
            world -> m_track->glVtx ( c->xyz, xLeft+2, yTop-1);
        }
    }

    glEnd () ;
    glEnable ( GL_TEXTURE_2D ) ;
}   // drawMap

//-----------------------------------------------------------------------------

// FIXME: is this still used? Afaik a special end page is displayed instead
void RaceGUI::drawGameOverText (const float dt)
{
    static float timer = 0 ;

    /* Calculate a color. This will result in an animation effect. */
    int red   = (int)(255 * sin ( (float)timer/5.1f ) / 2.0f + 0.5f);
    int green = (int)(255 * (sin ( (float)timer/6.3f ) / 2.0f + 0.5f));
    int blue  = (int)(255 * sin ( (float)timer/7.2f ) / 2.0f + 0.5f);
    timer += dt;

    assert(world != NULL);
    int finishing_position = world->getPlayerKart(0)->getPosition();

    if ( finishing_position > 1 )
    {
        char s[255];
        sprintf(s,_("YOU FINISHED %s"),m_pos_string[finishing_position]);
        font_race->PrintShadow(s, 64, 130, 300, red, green, blue);
    }
    else
    {
        font_race->PrintShadow(_("CONGRATULATIONS"),   64, 130, 300, 
                               red, green, blue);
        font_race->PrintShadow(_("YOU WON THE RACE!"), 64, 130, 210, 
                               red, green, blue);
    }
}   // drawGameOverText

//-----------------------------------------------------------------------------

// Draw players position on the race
void RaceGUI::drawPlayerIcons ()
{
    assert(world != NULL);

    int x = 5;
    int y;
#define ICON_WIDTH 40
#define ICON_PLAYER_WIDHT 50

    //glEnable(GL_TEXTURE_2D);
    Material *last_players_gst = 0;
    int   laps_of_leader           = -1;
    float time_of_leader       = -1;
    // Find the best time for the lap. We can't simply use
    // the time of the kart at position 1, since the kart
    // might have been overtaken by now
    for(unsigned int i = 0; i < world->getNumKarts() ; i++)
    {
        Kart* kart     = world->getKart(i);
        float lap_time = kart->getTimeAtLap();
        int laps       = kart->getLap();

        if(laps > laps_of_leader)
        {
            // more laps than current leader --> new leader and new time computation
            laps_of_leader = laps;
            time_of_leader = lap_time;
        } else if(laps == laps_of_leader)
        {
            // Same number of laps as leader: use fastest time
            time_of_leader=std::min(time_of_leader,lap_time);
        }
    }   // for i<getNumKarts

    int bFirst                 = 1;
    for(unsigned int i = 0; i < world->getNumKarts() ; i++)
    {
        Kart* kart   = world->getKart(i);
        int position = kart->getPosition();
        int lap      = kart->getLap();

        y = user_config->m_height*3/4-20 - ((position-1)*(ICON_PLAYER_WIDHT+2));

        // draw text
        int red=255, green=255, blue=255;
        int numLaps = world->m_race_setup.m_num_laps;
        if(lap>=numLaps)
        {  // kart is finished, display in green
            red=0; blue=0;
        }
        else if(lap>=0 && numLaps>1)
        {
            green = blue  = 255-(int)((float)lap/((float)numLaps-1.0f)*255.0f);
        }

        glDisable(GL_CULL_FACE);

        if(laps_of_leader>0 &&    // Display position during first lap
           (world->m_clock - kart->getTimeAtLap()<5.0f ||
            lap!=laps_of_leader))
        {  // Display for 5 seconds
            char str[256];
            if(position==1)
            {
                str[0]=' '; str[1]=0;
                TimeToString(kart->getTimeAtLap(), str+1);
            }
            else
            {
                float timeBehind;
                timeBehind = (lap==laps_of_leader ? kart->getTimeAtLap() : world->m_clock)
                    - time_of_leader;
                str[0]='+'; str[1]=0;
                TimeToString(timeBehind, str+1);
            }
            font_race->PrintShadow(str, 30, ICON_PLAYER_WIDHT+x, y+5,
                                   red, green, blue);
        }

        glEnable(GL_CULL_FACE);

        bFirst = 0;
        // draw icon
        Material* players_gst = kart->getKartProperties()->getIconMaterial();
        // Hmm - if the same icon is displayed more than once in a row,
        // plib does only do the first setTexture, therefore nothing is
        // displayed for the remaining icons. So we have to call force() if
        // the same icon is displayed more than once in a row.
        if(last_players_gst==players_gst)
        {
            players_gst->getState()->force();
        }
        //The material of the icons should not have a non-zero alpha_ref value,
        //because if so the next call can make the text look aliased.
        players_gst -> apply ();
        last_players_gst = players_gst;
        glBegin ( GL_QUADS ) ;
        glColor4f    ( 1, 1, 1, 1 ) ;
        if (kart -> isPlayerKart ())
        {
            glTexCoord2f ( 0, 0 ) ; glVertex2i ( x                  , y                   ) ;
            glTexCoord2f ( 1, 0 ) ; glVertex2i ( x+ICON_PLAYER_WIDHT, y                   ) ;
            glTexCoord2f ( 1, 1 ) ; glVertex2i ( x+ICON_PLAYER_WIDHT, y+ICON_PLAYER_WIDHT ) ;
            glTexCoord2f ( 0, 1 ) ; glVertex2i ( x                  , y+ICON_PLAYER_WIDHT ) ;
        }
        else
        {
            glTexCoord2f ( 0, 0 ) ; glVertex2i ( x           , y            ) ;
            glTexCoord2f ( 1, 0 ) ; glVertex2i ( x+ICON_WIDTH, y            ) ;
            glTexCoord2f ( 1, 1 ) ; glVertex2i ( x+ICON_WIDTH, y+ICON_WIDTH ) ;
            glTexCoord2f ( 0, 1 ) ; glVertex2i ( x           , y+ICON_WIDTH ) ;
        }
        glEnd () ;

        // draw position (1st, 2nd...)
        glDisable(GL_CULL_FACE);
        char str[256];

        sprintf(str, "%d", kart->getPosition());
        font_race->PrintShadow(str, 33, x-7, y-4);

        // FIXME: translation
        if (kart->getPosition() == 1)
            font_race->PrintShadow("st", 13, x-7+17, y-4+17);
        else if (kart->getPosition() == 2)
            font_race->PrintShadow("nd", 13, x-7+17, y-4+17);
        else if (kart->getPosition() == 3)
            font_race->PrintShadow("rd", 13, x-7+17, y-4+17);
        else
            font_race->PrintShadow("th", 13, x-7+17, y-4+17);

        glEnable(GL_CULL_FACE);
    }
}   // drawPlayerIcons

//-----------------------------------------------------------------------------
void RaceGUI::drawCollectableIcons ( Kart* player_kart, int offset_x,
                                     int offset_y, float ratio_x,
                                     float ratio_y                    )
{
    // If player doesn't have anything, do nothing.
    Collectable* collectable=player_kart->getCollectable();
    if(collectable->getType() == COLLECT_NOTHING) return;

    // Originally the hardcoded sizes were 320-32 and 400
    int x1 = (int)((user_config->m_width/2-32) * ratio_x) + offset_x ;
    int y1 = (int)(user_config->m_height*5/6 * ratio_y)      + offset_y;

    int nSize=(int)(64.0f*std::min(ratio_x, ratio_y));
    collectable->getIcon()->apply();

    int n  = player_kart->getNumCollectables() ;

    if ( n > 5 ) n = 5 ;
    if ( n < 1 ) n = 1 ;

    glBegin(GL_QUADS) ;
    glColor4f(1, 1, 1, 1 );

    for ( int i = 0 ; i < n ; i++ )
    {
        glTexCoord2f(0, 0); glVertex2i( i*30 + x1      , y1      );
        glTexCoord2f(1, 0); glVertex2i( i*30 + x1+nSize, y1      );
        glTexCoord2f(1, 1); glVertex2i( i*30 + x1+nSize, y1+nSize);
        glTexCoord2f(0, 1); glVertex2i( i*30 + x1      , y1+nSize);
    }   // for i
    glEnd () ;

}   // drawCollectableIcons

//-----------------------------------------------------------------------------
/* Energy meter that gets filled with coins */

// Meter fluid color (0 - 255)
#define METER_TOP_COLOR    240, 0, 0, 255
#define METER_BOTTOM_COLOR    240, 200, 0, 160
// Meter border color (0.0 - 1.0)
#define METER_BORDER_BLACK 0.0, 0.0, 0.0
#define METER_BORDER_WHITE 1.0, 1.0, 1.0

//-----------------------------------------------------------------------------
void RaceGUI::drawEnergyMeter ( Kart *player_kart, int offset_x, int offset_y,
                                float ratio_x, float ratio_y             )
{
    float state = (float)(player_kart->getNumHerring()) /
                  MAX_HERRING_EATEN;
    int x = (int)((user_config->m_width-24) * ratio_x) + offset_x;
    int y = (int)(250 * ratio_y) + offset_y;
    int w = (int)(16 * ratio_x);
    int h = (int)(user_config->m_height/4 * ratio_y);
    int wl = (int)(ratio_x);
    if(wl < 1)
        wl = 1;

    glDisable(GL_TEXTURE_2D);
    // Draw a Meter border
    x-=1;
    y-=1;
    // left side
    glBegin ( GL_QUADS ) ;
    glColor3f ( METER_BORDER_BLACK ) ;
    glVertex2i ( x-wl, y-wl ) ;
    glVertex2i ( x,    y-wl ) ;
    glVertex2i ( x,    y + h+1) ;
    glVertex2i ( x-wl, y + h+1) ;
    glEnd () ;

    // right side
    glBegin ( GL_QUADS ) ;
    glColor3f ( METER_BORDER_BLACK ) ;
    glVertex2i ( x+w,    y-wl ) ;
    glVertex2i ( x+w+wl, y-wl ) ;
    glVertex2i ( x+w+wl, y + h+1) ;
    glVertex2i ( x+w,    y + h+1) ;
    glEnd () ;

    // down side
    glBegin ( GL_QUADS ) ;
    glColor3f ( METER_BORDER_BLACK ) ;
    glVertex2i ( x,   y-wl ) ;
    glVertex2i ( x+w, y-wl ) ;
    glVertex2i ( x+w, y ) ;
    glVertex2i ( x,   y ) ;
    glEnd () ;

    // up side
    glBegin ( GL_QUADS ) ;
    glColor3f ( METER_BORDER_BLACK ) ;
    glVertex2i ( x,   y+h ) ;
    glVertex2i ( x+w, y+h ) ;
    glVertex2i ( x+w, y+h+wl ) ;
    glVertex2i ( x,   y+h+wl ) ;
    glEnd () ;

    x+=1;
    y+=1;

    // left side
    glBegin ( GL_QUADS ) ;
    glColor3f ( METER_BORDER_WHITE ) ;
    glVertex2i ( x-wl, y-wl ) ;
    glVertex2i ( x,    y-wl ) ;
    glVertex2i ( x,    y + h+1) ;
    glVertex2i ( x-wl, y + h+1) ;
    glEnd () ;

    // right side
    glBegin ( GL_QUADS ) ;
    glColor3f ( METER_BORDER_WHITE ) ;
    glVertex2i ( x+w,    y-wl ) ;
    glVertex2i ( x+w+wl, y-wl ) ;
    glVertex2i ( x+w+wl, y + h+1) ;
    glVertex2i ( x+w,    y + h+1) ;
    glEnd () ;

    // down side
    glBegin ( GL_QUADS ) ;
    glColor3f ( METER_BORDER_WHITE ) ;
    glVertex2i ( x,   y-wl ) ;
    glVertex2i ( x+w, y-wl ) ;
    glVertex2i ( x+w, y ) ;
    glVertex2i ( x,   y ) ;
    glEnd () ;

    // up side
    glBegin ( GL_QUADS ) ;
    glColor3f ( METER_BORDER_WHITE ) ;
    glVertex2i ( x,   y+h ) ;
    glVertex2i ( x+w, y+h ) ;
    glVertex2i ( x+w, y+h+wl ) ;
    glVertex2i ( x,   y+h+wl ) ;
    glEnd () ;

    // Draw the Meter fluid
    glBegin ( GL_QUADS ) ;
    glColor4ub ( METER_BOTTOM_COLOR ) ;
    glVertex2i ( x,   y ) ;
    glVertex2i ( x+w, y ) ;

    glColor4ub ( METER_TOP_COLOR ) ;
    glVertex2i ( x+w, y + (int)(state * h));
    glVertex2i ( x,   y + (int)(state * h) ) ;
    glEnd () ;
    glEnable(GL_TEXTURE_2D);
}   // drawEnergyMeter


//-----------------------------------------------------------------------------
void RaceGUI::drawSteering(Kart* kart, int offset_x, int offset_y,
                           float ratio_x, float ratio_y           )
{

    float minRatio = std::min(ratio_x, ratio_y);
#define WHEELWIDTH 64
    int width  = (int)(WHEELWIDTH*minRatio);
    int height = (int)(WHEELWIDTH*minRatio);
    offset_x += (int)((user_config->m_width-160)*ratio_x) - width;
    offset_y += (int)(6*ratio_y);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    // for now we display the maximum steering as a 45 degree angle.
    // One the steering angle for all karts are fixed, this should be
    // changed, so that the user gets feedback about how much steering
    // is currently done, since it will vary from kart to kart.
    float displayedAngle = 45.0f * kart->getSteerPercent();

    int tw = width/2; int th = height/2;
    glTranslatef( offset_x+tw,  offset_y+th, 0.0f);
    glRotatef(displayedAngle, 0.0f, 0.0f, 1.0f);
    glTranslatef(-offset_x-tw, -offset_y-th, 0.0f);

    m_steering_wheel_icon->getState()->force();
    glBegin ( GL_QUADS ) ;
    glColor4f    ( 1, 1, 1, 1 ) ;
    glTexCoord2f(0, 0);glVertex2i(offset_x      , offset_y       );
    glTexCoord2f(1, 0);glVertex2i(offset_x+width, offset_y       );
    glTexCoord2f(1, 1);glVertex2i(offset_x+width, offset_y+height);
    glTexCoord2f(0, 1);glVertex2i(offset_x      , offset_y+height);
    glEnd () ;

    glPopMatrix();
} // drawSteering

//-----------------------------------------------------------------------------
void RaceGUI::drawPosition(Kart* kart, int offset_x, int offset_y,
                           float ratio_x, float ratio_y           )
{

    char str[256];
    offset_x += (int)((user_config->m_width-110)*ratio_x);
    offset_y += (int)(140*ratio_y);

    sprintf(str, "%d", kart->getPosition());
    font_race->PrintShadow(str, (int)(100*ratio_y), offset_x, offset_y);

    offset_x += (int)(50*ratio_x);
    offset_y += (int)(50*ratio_y);

    // FIXME: translation
    if (kart->getPosition() == 1)
        font_race->PrintShadow("st", (int)(40*ratio_y), offset_x, offset_y);
    else if (kart->getPosition() == 2)
        font_race->PrintShadow("nd", (int)(40*ratio_y), offset_x, offset_y);
    else if (kart->getPosition() == 3)
        font_race->PrintShadow("rd", (int)(40*ratio_y), offset_x, offset_y);
    else
        font_race->PrintShadow("th", (int)(40*ratio_y), offset_x, offset_y);
} // drawPosition

//-----------------------------------------------------------------------------
void RaceGUI::drawSpeed(Kart* kart, int offset_x, int offset_y,
                        float ratio_x, float ratio_y           )
{

    float minRatio = std::min(ratio_x, ratio_y);
#define SPEEDWIDTH 128
    int width  = (int)(SPEEDWIDTH*minRatio);
    int height = (int)(SPEEDWIDTH*minRatio);
    offset_x += (int)((user_config->m_width-10)*ratio_x) - width;
    offset_y += (int)(10*ratio_y);

    glMatrixMode(GL_MODELVIEW);
    m_speed_back_icon->getState()->force();
    glBegin ( GL_QUADS ) ;
    glTexCoord2f(0, 0);glVertex2i(offset_x      , offset_y       );
    glTexCoord2f(1, 0);glVertex2i(offset_x+width, offset_y       );
    glTexCoord2f(1, 1);glVertex2i(offset_x+width, offset_y+height);
    glTexCoord2f(0, 1);glVertex2i(offset_x      , offset_y+height);
    glEnd () ;

#ifdef BULLET
    //convention taken from btRaycastVehicle::updateVehicle
    const float speed =  kart->getSpeed();
#else
    const float speed = kart->getVelocity()->xyz[1];
#endif

    if ( !kart->isOnGround() )
        font_race->PrintShadow("!", (int)(60*minRatio), 
                               offset_x-(int)(30*minRatio), 
                               offset_y-(int)(10*minRatio));
    /* Show speed */
    if ( speed < 0 )
        font_race->PrintShadow(_("REV"), (int)(40*minRatio), 
                               offset_x+(int)(40*minRatio), 
                               offset_y+(int)(10*minRatio));
    else
    {
        if ( speed >= kart->getMaxSpeed()*kart->getWheelieMaxSpeedRatio() )
        {
            font_race->PrintShadow("l", (int)(60*minRatio), 
                                   offset_x+(int)(70*minRatio), offset_y);
            font_race->PrintShadow("^", (int)(60*minRatio), 
                                   offset_x+(int)(65*minRatio), 
                                   offset_y+(int)(7*minRatio));
        }

        float speedRatio = speed/KILOMETERS_PER_HOUR/110.0f;
        // The following does not work with wheelie or Zipper
        //float speedRatio = kart->getVelocity()->xyz[1]/(kart->getMaxSpeed();

        if ( speedRatio > 1 )
            speedRatio = 1;

        m_speed_fore_icon->getState()->force();
        glBegin ( GL_POLYGON ) ;
        glTexCoord2f(1, 0);glVertex2i(offset_x+width, offset_y);
        glTexCoord2f(0, 0);glVertex2i(offset_x, offset_y);
        if (speedRatio < 0.5)
        {
            glTexCoord2f(0, speedRatio*2);glVertex2i(offset_x, (int)(offset_y+width*speedRatio*2));
        }
        else
        {
            glTexCoord2f(0, 1);glVertex2i(offset_x, offset_y+width);
            glTexCoord2f((speedRatio-0.5)*2, 1);glVertex2i((int)(offset_x+height*(speedRatio-0.5)*2), offset_y+height);
        }

        glEnd () ;
    }   // speed<0

} // drawSpeed

//-----------------------------------------------------------------------------
void RaceGUI::drawLap(Kart* kart, int offset_x, int offset_y,
                      float ratio_x, float ratio_y           )
{

    float maxRatio = std::max(ratio_x, ratio_y);
    char str[256];
    offset_x += (int)(120*ratio_x);
    offset_y += (int)(50*maxRatio);

    if ( kart->getLap() >= world->m_race_setup.m_num_laps )
    {
        sprintf(str, _("Finished"));
        font_race->PrintShadow(str, (int)(48*maxRatio), offset_x, offset_y);
    }
    else
    {
        font_race->PrintShadow( _("Lap"), (int)(48*maxRatio), offset_x, offset_y);

        offset_y -= (int)(50*ratio_y);

        sprintf(str, "%d/%d", kart->getLap()<0?0:kart->getLap()+1, 
                world->m_race_setup.m_num_laps);
        font_race->PrintShadow(str, (int)(48*maxRatio), offset_x, offset_y);
    }
} // drawLap

//-----------------------------------------------------------------------------
/** Removes messages which have been displayed long enough. This function
 *  must be called after drawAllMessages, otherwise messages which are onlu
 *  displayed once will not be drawn!
 **/

void RaceGUI::cleanupMessages()
{
    AllMessageType::iterator p =m_messages.begin(); 
    while(p!=m_messages.end())
    {
        if((*p)->done())
            m_messages.erase(p);
        else
            p++;
    }
}   // cleanupMessages

//-----------------------------------------------------------------------------
/** Displays all messages in the message queue
 **/
void RaceGUI::drawAllMessages(Kart* player_kart, int offset_x, int offset_y,
                              float ratio_x,  float ratio_y  )
{
    int x, y;
    x = SCREEN_CENTERED_TEXT;
    // First line of text somewhat under the top of the screen. For now
    // start just under the timer display
    y = (int)(ratio_y*(user_config->m_height -164)+offset_y);
    // The message are displayed in reverse order, so that a multi-line
    // message (addMessage("1", ...); addMessage("2",...) is displayed
    // in the right order: "1" on top of "2"
    for(std::vector<TimedMessage*>::iterator i=m_messages.begin();
                                             i!=m_messages.end(); i++)
    {
        // Display only messages for all karts, or messages for this kart
        if( (*i)->m_kart && (*i)->m_kart!=player_kart) continue;

        font_race->Print( (*i)->m_message, (*i)->m_font_size, 
                          Font::ALIGN_CENTER, Font::CENTER_OF_SCREEN, 
                          Font::ALIGN_BOTTOM, y,
                          (*i)->m_red, (*i)->m_green, (*i)->m_blue,
                          ratio_x, ratio_y,
                          offset_x, offset_x+(int)(user_config->m_width*ratio_x));
        // Add 20% of font size as space between the lines
        y-=(*i)->m_font_size*12/10;
        
        
    }   // for i in all messages
}   // drawAllMessages

//-----------------------------------------------------------------------------
/** Adds a message to the message queue. The message is displayed for a
 *  certain amount of time (unless time<0, then the message is displayed
 *  once).
 **/
void RaceGUI::addMessage(const char *msg, Kart *kart, float time, 
                         int font_size, int red, int green, int blue)
{
    TimedMessage *m=new TimedMessage(msg, kart, time, font_size, 
                                     red, green, blue);
    m_messages.push_back(m);
}   // addMessage

//-----------------------------------------------------------------------------
void RaceGUI::drawStatusText (const RaceSetup& raceSetup, const float dt)
{
    assert(world != NULL);

    glMatrixMode   ( GL_MODELVIEW ) ;
    glPushMatrix   () ;
    glLoadIdentity () ;

    glMatrixMode   ( GL_PROJECTION ) ;
    glPushMatrix   () ;
    glLoadIdentity () ;

    glPushAttrib   ( GL_ENABLE_BIT | GL_LIGHTING_BIT ) ;
    glDisable      ( GL_DEPTH_TEST   );
    glDisable      ( GL_LIGHTING     );
    glDisable      ( GL_FOG          );
    glDisable      ( GL_CULL_FACE    );
    glEnable       ( GL_ALPHA_TEST   );
    glAlphaFunc    ( GL_GREATER, 0.1f);
    glEnable       ( GL_BLEND        );

    glOrtho        ( 0, user_config->m_width, 0, user_config->m_height, 0, 100 ) ;
    switch (world->m_ready_set_go)
    {
    case 2: font_race->PrintShadow(_("Ready!"), 90, 
                                   Font::ALIGN_CENTER, Font::CENTER_OF_SCREEN, 
                                   Font::ALIGN_CENTER, Font::CENTER_OF_SCREEN, 
                                   230, 170, 160);
        break;
    case 1: font_race->PrintShadow(_("Set!"), 90, 
                                   Font::ALIGN_CENTER, Font::CENTER_OF_SCREEN, 
                                   Font::ALIGN_CENTER, Font::CENTER_OF_SCREEN,
                                   230, 230, 160);
        break;
    case 0: font_race->PrintShadow(_("Go!"), 90, 
                                   Font::ALIGN_CENTER, Font::CENTER_OF_SCREEN, 
                                   Font::ALIGN_CENTER, Font::CENTER_OF_SCREEN,
                                   100, 210, 100);
        break;
    }   // switch

    for(int i = 0; i < 10; ++i)
    {
        if(world->m_debug_text[i] != "")
            font_race->Print(world->m_debug_text[i].c_str(),
                             20, 20, 200 -i*20, 100, 210, 100);
    }
    if(world->getPhase()==World::START_PHASE)
    {
        for(int i=0; i<raceSetup.getNumPlayers(); i++)
        {
            if(world->getPlayerKart(i)->earlyStartPenalty())
            {
                font_race->PrintShadow(_("Penalty time!!"), 80,
                                       Font::ALIGN_CENTER, Font::CENTER_OF_SCREEN, 
                                       Font::ALIGN_BOTTOM, 200, 200, 10, 10);
            }   // if penalty
        }  // for i < getNumPlayers
    }  // if not RACE_PHASE

    float split_screen_ratio_x, split_screen_ratio_y;
    split_screen_ratio_x = split_screen_ratio_y = 1.0;
    if(raceSetup.getNumPlayers() >= 2)
        split_screen_ratio_y = 0.5;
    if(raceSetup.getNumPlayers() >= 3)
        split_screen_ratio_x = 0.5;

    if ( world->getPhase() == World::FINISH_PHASE )
    {
        // FIXME: is this still used? It should be replaced by a special
        //        end page.
        drawGameOverText(dt) ;
    }   // if FINISH_PHASE
    if ( world->getPhase() == World::RACE_PHASE         ||
         world->getPhase() == World::DELAY_FINISH_PHASE   )
    {
        for(int pla = 0; pla < raceSetup.getNumPlayers(); pla++)
        {
            int offset_x, offset_y;
            offset_x = offset_y = 0;

            if(raceSetup.getNumPlayers() == 2)
            {
                if(pla == 0) offset_y = user_config->m_height/2;
            }
            else if(raceSetup.getNumPlayers() > 2)
            {
                if((pla == 0 && raceSetup.getNumPlayers() > 1) || (pla == 1))
                    offset_y = user_config->m_height/2;

                if((pla == 1) || pla == 3)
                    offset_x = user_config->m_width/2;
            }

            Kart* player_kart=world->getPlayerKart(pla);
            drawCollectableIcons(player_kart, offset_x, offset_y,
                                 split_screen_ratio_x, split_screen_ratio_y );
            drawEnergyMeter     (player_kart, offset_x, offset_y,
                                 split_screen_ratio_x, split_screen_ratio_y );
            drawSteering        (player_kart, offset_x, offset_y,
                                 split_screen_ratio_x, split_screen_ratio_y );
            drawPosition        (player_kart, offset_x, offset_y,
                                 split_screen_ratio_x, split_screen_ratio_y );
            drawSpeed           (player_kart, offset_x, offset_y,
                                 split_screen_ratio_x, split_screen_ratio_y );
            drawLap             (player_kart, offset_x, offset_y,
                                 split_screen_ratio_x, split_screen_ratio_y );
            drawAllMessages     (player_kart, offset_x, offset_y,
                                 split_screen_ratio_x, split_screen_ratio_y );
        }   // for pla
        drawTimer ();
        drawMap   ();
        if ( user_config->m_display_fps ) drawFPS ();
        drawPlayerIcons() ;
    }   // if RACE_PHASE

    glPopAttrib  () ;
    glPopMatrix  () ;
    glMatrixMode ( GL_MODELVIEW ) ;
    glPopMatrix  () ;
}   // drawStatusText

/* EOF */
