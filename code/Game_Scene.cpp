/*
 * GAME SCENE
 * Copyright © 2020+ Daniel Sanchez Gamo
 *
 * Distributed under the Boost Software License, version  1.0
 * See documents/LICENSE.TXT or www.boost.org/LICENSE_1_0.txt
 *
 * danielsanchezgamo@gmail.com
 */

#include "Game_Scene.hpp"

#include <cstdlib>
#include <basics/Canvas>
#include <basics/Director>

using namespace basics;
using namespace std;

namespace example
{
    // ---------------------------------------------------------------------------------------------
    // ID y ruta de las texturas que se deben cargar para esta escena. La textura con el mensaje de
    // carga está la primera para poder dibujarla cuanto antes:

    Game_Scene::Texture_Data Game_Scene::textures_data[] =
    {
        { ID(loading),    "game-scene/loading.png"        },
        { ID(hbar),       "game-scene/horizontal-bar.png" },
        { ID(vbar),       "game-scene/vertical-bar.png"   },
        { ID(player-bar), "game-scene/players-bar.png"    },
        { ID(ball),       "game-scene/ball.png"           },

        { ID(frog),       "game-scene/frog.png"   },
            { ID(truck),       "game-scene/truck.png"           },

        { ID(carretera),       "game-scene/road.png"           },
        { ID(hierba),       "game-scene/grass.png"           },
        { ID(meta),       "game-scene/meta.png"           },
        { ID(agua),       "game-scene/water.png"           },
        { ID(coche1),       "game-scene/car1.png"           },
        { ID(coche2),       "game-scene/car2.png"           },
        { ID(coche3),       "game-scene/car3.png"           },
        { ID(troncogrande),       "game-scene/biglog.png"           },
        { ID(troncopequeno),       "game-scene/logsmall.png"           },

        { ID(tortugagrande),       "game-scene/bigturtles.png"           },
        { ID(tortugapequena),       "game-scene/smallturtles.png"           },


        { ID(flechan),       "game-scene/arrowtop.png"           },
        { ID(flechas),       "game-scene/arrowbottom.png"           },
        { ID(flechae),       "game-scene/arrowright.png"           },
        { ID(flechao),       "game-scene/arrowleft.png"           },
    };

    // Pâra determinar el número de items en el array textures_data, se divide el tamaño en bytes
    // del array completo entre el tamaño en bytes de un item:

    unsigned Game_Scene::textures_count = sizeof(textures_data) / sizeof(Texture_Data);

    // ---------------------------------------------------------------------------------------------
    // Definiciones de los atributos estáticos de la clase:
    constexpr float Game_Scene::  car1_speed;
    constexpr float Game_Scene::  car2_speed;
    constexpr float Game_Scene::  truck_speed;
    constexpr float Game_Scene::  ball_speed;
    constexpr float Game_Scene::player_speed;

    // ---------------------------------------------------------------------------------------------

    Game_Scene::Game_Scene()
    {
        // Se establece la resolución virtual (independiente de la resolución virtual del dispositivo).
        // En este caso no se hace ajuste de aspect ratio, por lo que puede haber distorsión cuando
        // el aspect ratio real de la pantalla del dispositivo es distinto.

        canvas_width  = 720;
        canvas_height =  1280;

        // Se inicia la semilla del generador de números aleatorios:

        srand (unsigned(time(nullptr)));

        // Se inicializan otros atributos:

        initialize ();
    }

    // ---------------------------------------------------------------------------------------------
    // Algunos atributos se inicializan en este método en lugar de hacerlo en el constructor porque
    // este método puede ser llamado más veces para restablecer el estado de la escena y el constructor
    // solo se invoca una vez.

    bool Game_Scene::initialize ()
    {
        state     = LOADING;
        suspended = true;
        gameplay  = UNINITIALIZED;

        return true;
    }

    // ---------------------------------------------------------------------------------------------

    void Game_Scene::suspend ()
    {
        suspended = true;               // Se marca que la escena ha pasado a primer plano
    }

    // ---------------------------------------------------------------------------------------------

    void Game_Scene::resume ()
    {
        suspended = false;              // Se marca que la escena ha pasado a segundo plano
    }

    // ---------------------------------------------------------------------------------------------

    void Game_Scene::handle (Event & event)
    {
        if (state == RUNNING)               // Se descartan los eventos cuando la escena está LOADING
        {
            if (gameplay == WAITING_TO_START)
            {

                start_playing ();           // Se empieza a jugar cuando el usuario toca la pantalla por primera vez
            }
            else switch (event.id)
            {
                case ID(touch-started):     // El usuario toca la pantalla
                {
                    touch_location = { *event[ID(x)].as< var::Float > (), *event[ID(y)].as< var::Float > () };


                    user_target_x = *event[ID(x)].as< var::Float > ();
                    user_target_y = *event[ID(y)].as< var::Float > ();


                    follow_target = true;
                    break;
                }

                case ID(touch-moved):

                {

                    break;
                }

                case ID(touch-ended):       // El usuario deja de tocar la pantalla
                {

                    follow_target = false;
                    break;
                }
            }
        }
    }

    // ---------------------------------------------------------------------------------------------

    void Game_Scene::update (float time)
    {
        if (!suspended) switch (state)
        {
            case LOADING: load_textures  ();     break;
            case RUNNING: run_simulation (time); break;
            case ERROR:   break;
        }
    }

    // ---------------------------------------------------------------------------------------------

    void Game_Scene::render (Context & context)
    {
        if (!suspended)
        {
            // El canvas se puede haber creado previamente, en cuyo caso solo hay que pedirlo:

            Canvas * canvas = context->get_renderer< Canvas > (ID(canvas));

            // Si no se ha creado previamente, hay que crearlo una vez:

            if (!canvas)
            {
                 canvas = Canvas::create (ID(canvas), context, {{ canvas_width, canvas_height }});
            }

            // Si el canvas se ha podido obtener o crear, se puede dibujar con él:

            if (canvas)
            {
                canvas->clear ();

                switch (state)
                {
                    case LOADING: render_loading   (*canvas); break;
                    case RUNNING: render_playfield (*canvas); break;
                    case ERROR:   break;
                }
            }
        }
    }

    // ---------------------------------------------------------------------------------------------
    // En este método solo se carga una textura por fotograma para poder pausar la carga si el
    // juego pasa a segundo plano inesperadamente. Otro aspecto interesante es que la carga no
    // comienza hasta que la escena se inicia para así tener la posibilidad de mostrar al usuario
    // que la carga está en curso en lugar de tener una pantalla en negro que no responde durante
    // un tiempo.

    void Game_Scene::load_textures ()
    {
        if (textures.size () < textures_count)          // Si quedan texturas por cargar...
        {
            // Las texturas se cargan y se suben al contexto gráfico, por lo que es necesario disponer
            // de uno:

            Graphics_Context::Accessor context = director.lock_graphics_context ();

            if (context)
            {
                // Se carga la siguiente textura (textures.size() indica cuántas llevamos cargadas):

                Texture_Data   & texture_data = textures_data[textures.size ()];
                Texture_Handle & texture      = textures[texture_data.id] = Texture_2D::create (texture_data.id, context, texture_data.path);

                // Se comprueba si la textura se ha podido cargar correctamente:

                if (texture) context->add (texture); else state = ERROR;

                // Cuando se han terminado de cargar todas las texturas se pueden crear los sprites que
                // las usarán e iniciar el juego:
            }
        }
        else
        if (timer.get_elapsed_seconds () > 1.f)         // Si las texturas se han cargado muy rápido
        {                                               // se espera un segundo desde el inicio de
            create_sprites ();                          // la carga antes de pasar al juego para que
            restart_game   ();                          // el mensaje de carga no aparezca y desaparezca
                                                        // demasiado rápido.
            state = RUNNING;
        }
    }

    // ---------------------------------------------------------------------------------------------

    void Game_Scene::create_sprites ()
    {
        // Se crean y configuran los sprites del fondo:


        Sprite_Handle    tbutton(new Sprite( textures[ID(flechan)].get () ));
        Sprite_Handle    bbutton(new Sprite( textures[ID(flechas)].get () ));
       Sprite_Handle rbutton(new Sprite( textures[ID(flechae)].get () ));
        Sprite_Handle lbutton(new Sprite( textures[ID(flechao)].get () ));

        Sprite_Handle    top_bar(new Sprite( textures[ID(hbar)].get () ));
        Sprite_Handle left_bar(new Sprite( textures[ID(vbar)].get () ));
        Sprite_Handle right_bar(new Sprite( textures[ID(vbar)].get () ));
        Sprite_Handle bottom_bar(new Sprite( textures[ID(hbar)].get () ));

        Sprite_Handle    bggrassmiddle(new Sprite( textures[ID(hierba)].get () ));
        Sprite_Handle    bggrassbottom(new Sprite( textures[ID(hierba)].get () ));
        Sprite_Handle    grassgoal(new Sprite( textures[ID(meta)].get () ));
        Sprite_Handle bgroad(new Sprite( textures[ID(carretera)].get () ));
        Sprite_Handle bgwater(new Sprite( textures[ID(agua)].get () ));

       lbutton->set_position ({canvas_width/1.42f, canvas_height / 30.f });
        rbutton->set_position ({canvas_width/1.1f, canvas_height /  30.f });
        tbutton->set_position ({canvas_width/10.f, canvas_height /  30.f });
     bbutton->set_position ({canvas_width/3.33f, canvas_height /  30.f });






        top_bar->set_anchor   (TOP | LEFT);
           top_bar->set_position ({ 0, canvas_height });


        left_bar->set_anchor   (LEFT);

        left_bar->set_position ({ -300, canvas_height / 2.f });

        right_bar->set_anchor   (LEFT);
        right_bar->set_position ({ canvas_width+300, canvas_height / 2.f });

        bottom_bar->set_anchor   (LEFT);
        bottom_bar->set_position ({ 0, canvas_height / 15.15f });



        bgwater->set_anchor   ( CENTER);
        bgwater->set_position ({ canvas_width/2.f, canvas_height /1.428f });


        bggrassmiddle->set_anchor   (CENTER);
        bggrassmiddle->set_position ({ canvas_width/2.f, canvas_height/2 });

        bgroad->set_anchor   (CENTER);
        bgroad->set_position ({ canvas_width / 2.f, canvas_height / 3.32f });



        bggrassbottom->set_anchor   (BOTTOM);
        bggrassbottom->set_position ({ canvas_width/2.f, canvas_height / 14.f });


        grassgoal->set_anchor   (BOTTOM);
        grassgoal->set_position ({ canvas_width/2.f, canvas_height / 1.152f });

        sprites.push_back (bggrassbottom);
        sprites.push_back (bggrassmiddle);
        sprites.push_back (bgroad);
        sprites.push_back (bgwater);
        sprites.push_back (grassgoal);


        sprites.push_back (  top_bar);
        sprites.push_back (   left_bar);
        sprites.push_back (   right_bar);

        sprites.push_back (bottom_bar);

        sprites.push_back (  tbutton);
        sprites.push_back (   lbutton);
        sprites.push_back (   rbutton);
       sprites.push_back (bbutton);

       //Se crean el player y los elementos





        Sprite_Handle caryellow1_handle(new Sprite( textures[ID(coche1)].get () ));
        Sprite_Handle caryellow2_handle(new Sprite( textures[ID(coche1)].get () ));

        Sprite_Handle carblue1_handle(new Sprite( textures[ID(coche3)].get () ));
        Sprite_Handle carblue2_handle(new Sprite( textures[ID(coche3)].get () ));

        Sprite_Handle carwhite1_handle(new Sprite( textures[ID(coche2)].get () ));
        Sprite_Handle carwhite2_handle(new Sprite( textures[ID(coche2)].get () ));


        Sprite_Handle truckml1_handle(new Sprite( textures[ID(truck)].get () ));


        Sprite_Handle truckll1_handle(new Sprite( textures[ID(truck)].get () ));
;
        Sprite_Handle biglog1ll_handle(new Sprite( textures[ID(troncogrande)].get () ));

        Sprite_Handle biglog1ml_handle(new Sprite( textures[ID(troncogrande)].get () ));


        Sprite_Handle smalllog1_handle(new Sprite( textures[ID(troncopequeno)].get () ));
        Sprite_Handle smalllog2_handle(new Sprite( textures[ID(troncopequeno)].get () ));


        Sprite_Handle bigturtle1_handle(new Sprite( textures[ID(tortugagrande)].get () ));
        Sprite_Handle bigturtle2_handle(new Sprite( textures[ID(tortugagrande)].get () ));

        Sprite_Handle smallturtle1_handle(new Sprite( textures[ID(tortugapequena)].get () ));
        Sprite_Handle smallturtle2_handle(new Sprite( textures[ID(tortugapequena)].get () ));

        Sprite_Handle right_player_handle(new Sprite( textures[ID(frog)].get () ));




        sprites.push_back (        truckll1_handle);

        sprites.push_back (        truckml1_handle);



        sprites.push_back (        caryellow1_handle);
        sprites.push_back (        caryellow2_handle);


        sprites.push_back (        carblue1_handle);
        sprites.push_back (        carblue2_handle);


        sprites.push_back (        carwhite1_handle);
        sprites.push_back (        carwhite2_handle);


        sprites.push_back (        biglog1ml_handle);


        sprites.push_back (        biglog1ll_handle);


        sprites.push_back (        smalllog1_handle);
        sprites.push_back (        smalllog2_handle);

        sprites.push_back (        bigturtle1_handle);
        sprites.push_back (        bigturtle2_handle);

        sprites.push_back (        smallturtle1_handle);
        sprites.push_back (        smallturtle2_handle);
        sprites.push_back (right_player_handle);



        // Se guardan punteros a los sprites que se van a usar frecuentemente:

        top_border    =             top_bar.get ();
        bottom_border =          bottom_bar.get ();
        left_border      =         left_bar.get ();
        right_border      =         right_bar.get ();
        right_player  = right_player_handle.get ();
larrow = lbutton.get();
rarrow = rbutton.get();
tarrow = tbutton.get();
barrow = bbutton.get();
        meta = grassgoal.get();





       truckmidlane1 = truckml1_handle.get();


        trucklastlane1 = truckll1_handle.get();


        caryellow1          = caryellow1_handle.get();
        caryellow2          = caryellow2_handle.get();


        carblue1          = carblue1_handle.get();
        carblue2          = carblue2_handle.get();


        carwhite1          = carwhite1_handle.get();
        carwhite2          = carwhite2_handle.get();





        smalllog1          = smalllog1_handle.get();
        smalllog2          = smalllog2_handle.get();


        biglog1ml          = biglog1ml_handle.get();


        biglog1ll          = biglog1ll_handle.get();




        smallturtle1          = smallturtle1_handle.get();
        smallturtle2          = smallturtle2_handle.get();


        bigturtle1          = bigturtle1_handle.get();
        bigturtle2        = bigturtle2_handle.get();




    }

    // ---------------------------------------------------------------------------------------------
    // Juando el juego se inicia por primera vez o cuando se reinicia porque un jugador pierde, se
    // llama a este método para restablecer la posición y velocidad de los sprites:

    void Game_Scene::restart_game()
    {

        follow_target=false;





        carblue1->set_position ({canvas_width/1.33f, canvas_height / 6.02f });
        carblue1->set_speed({ -1.f, 0.f });
        carblue2->set_position ({canvas_width/4.f, canvas_height / 6.02f });
        carblue2->set_speed({ -1.f, 0.f });
        carblue1->set_tag("vehiculo");
        carblue2->set_tag("vehiculo");


        truckmidlane1->set_position ({canvas_width/4.f, canvas_height / 3.32f });
        truckmidlane1->set_speed({ 1.f, 0.f });


        truckmidlane1->set_tag("vehiculo");


        trucklastlane1->set_position ({canvas_width/1.33f, canvas_height / 2.30f });
        trucklastlane1->set_speed({ 1.f, 0.f });


        trucklastlane1->set_tag("vehiculo");


        caryellow1->set_position ({canvas_width/1.33f, canvas_height / 2.73f });
        caryellow1->set_speed({ -1.f, 0.f });
        caryellow2->set_position ({canvas_width/4.f, canvas_height / 2.73f });
        caryellow2->set_speed({ -1.f, 0.f });


        caryellow1->set_tag("vehiculo");
        caryellow2->set_tag("vehiculo");




        carwhite1->set_position ({canvas_width/1.33f, canvas_height / 4.27f });
        carwhite1->set_speed({ -1.f, 0.f });
        carwhite2->set_position ({canvas_width/4.f, canvas_height / 4.27f });



        carwhite1->set_tag("vehiculo");
        carwhite2->set_tag("vehiculo");





        smalllog1->set_position ({canvas_width/6.f, canvas_height / 1.57f });
        smalllog1->set_speed({ -1.f, 0.f });
        smalllog2->set_position ({canvas_width/2.f, canvas_height / 1.57f });
        smalllog2->set_speed({ -1.f, 0.f });


        smalllog1->set_tag("transporte");
        smalllog2->set_tag("transporte");


        biglog1ml->set_position ({canvas_width/1.33f, canvas_height / 1.42f });
        biglog1ml->set_speed({ 1.f, 0.f });


        biglog1ml->set_tag("transporte");




        biglog1ll->set_position ({canvas_width/4.f, canvas_height / 1.19f });
        biglog1ll->set_speed({ 1.f, 0.f });


        biglog1ll->set_tag("transporte");


        smallturtle1->set_position ({canvas_width/6.f, canvas_height / 1.76f });
        smallturtle1->set_speed({ -1.f, 0.f });
        smallturtle2->set_position ({canvas_width/1.19f, canvas_height / 1.76f });
        smallturtle2->set_speed({ -1.f, 0.f });


        smallturtle1->set_tag("transporte");
        smallturtle2->set_tag("transporte");


        bigturtle1->set_position ({canvas_width/1.33f, canvas_height / 1.30f });
        bigturtle1->set_speed({ 1.f, 0.f });
        bigturtle2->set_position ({canvas_width/4.f, canvas_height / 1.30f });
        bigturtle2->set_speed({ 1.f, 0.f });

        bigturtle1->set_tag("transporte");
        bigturtle2->set_tag("transporte");

        right_player->set_position ({ canvas_width / 2.f  , canvas_height / 10.f });
        right_player->set_speed({ 0.f, 0.f });

        gameplay = WAITING_TO_START;
    }

    // ---------------------------------------------------------------------------------------------

    void Game_Scene::start_playing ()
    {
        // Se genera un vector de dirección al azar:

        Vector2f random_direction
        (
            float(rand () % int(canvas_width ) - int(canvas_width  / 2)),
            float(rand () % int(canvas_height) - int(canvas_height / 2))
        );

        // Se hace unitario el vector y se multiplica un el valor de velocidad para que el vector
        // resultante tenga exactamente esa longitud:


        //MOVIMIENTOS LATERALES DE LOS COCHES


        caryellow1->set_speed_x (-car3_speed);
        caryellow2->set_speed_x (-car3_speed);


        carwhite1->set_speed_x (-car2_speed);
        carwhite2->set_speed_x (-car2_speed);


        carblue1->set_speed_x (-car1_speed);
        carblue2->set_speed_x (-car1_speed);

        truckmidlane1->set_speed_x (truck_speed);

        trucklastlane1->set_speed_x (truck_speed);

        smalllog1->set_speed_x (+car2_speed);
        smalllog2->set_speed_x (+car2_speed);


        biglog1ml->set_speed_x (truck_speed);


        biglog1ll->set_speed_x (truck_speed);

        smallturtle1->set_speed_x (-car1_speed);
                smallturtle2->set_speed_x (-car1_speed);


                bigturtle1->set_speed_x (-car1_speed);
        bigturtle2->set_speed_x (-car1_speed);




        gameplay = PLAYING;
    }

    // ---------------------------------------------------------------------------------------------

    void Game_Scene::run_simulation (float time)
    {
        // Se actualiza el estado de todos los sprites:

        for (auto & sprite : sprites)
        {
            sprite->update (time);
        }

        update_ai   ();
        update_user ();

        // Se comprueban las posibles colisiones de la bola con los bordes y con los players:

        check_ball_collisions ();
    }

    // ---------------------------------------------------------------------------------------------
    // Usando un algoritmo sencillo se controla automáticamente el comportamiento del jugador
    // izquierdo.

    void Game_Scene::update_ai () {

        if (biglog1ml->contains(right_player->get_position())||biglog1ll->contains(right_player->get_position()))
        {
            right_player->set_speed_x (truck_speed);
        }

        if (smalllog1->contains(right_player->get_position())||smalllog2->contains(right_player->get_position()))
        {
            right_player->set_speed_x (car2_speed);
        }

        if (smallturtle1->contains(right_player->get_position())||smallturtle2->contains(right_player->get_position())
        ||bigturtle1->contains(right_player->get_position())||bigturtle2->contains(right_player->get_position())
        )
        {
            right_player->set_speed_x (-car1_speed);
        }





    }

    // ---------------------------------------------------------------------------------------------
    // Se hace que el player dechero se mueva hacia arriba o hacia abajo según el usuario esté
    // tocando la pantalla por encima o por debajo de su centro. Cuando el usuario no toca la
    // pantalla se deja al player quieto.

    void Game_Scene::update_user ()
    {


     if (right_player->intersects (*caryellow1)||right_player->intersects (*caryellow2)
     ||right_player->intersects (*carblue1)||right_player->intersects (*carblue2)
     ||right_player->intersects (*carwhite1)||right_player->intersects (*carwhite2)
      ||right_player->intersects (*truckmidlane1)||right_player->intersects (*trucklastlane1)
     ) {
restart_game();
     }



        if (right_player->intersects (*meta)) {
            restart_game();

        }



          //  si el jugador toca al agua  y no toca los sprites de tortuga o tronco se reinicia tambien
      /*
       if (right_player->intersects (*tronco)) {

         rightplayer.
         rightplayer pilla la misma velocidad que el tronco excepto si toca con algun borde
     }
      *
      * */



        if (right_player->intersects (*top_border))
        {
            // Si el player está tocando el borde superior, no puede ascender:

            right_player->set_position_y (top_border->get_bottom_y () - right_player->get_height () / 2.f);
            right_player->set_speed_y (0);
        }
        else
        if (right_player->intersects (*bottom_border))
        {
            // Si el player está tocando el borde inferior, no puede descender:

            right_player->set_position_y (bottom_border->get_top_y () + right_player->get_height () / 2.f);
            right_player->set_speed_y (0);
        }
        else
        if (follow_target)
        {
            // Si el usuario está tocando la pantalla, se determina si está tocando por encima o por
            // debajo de su centro para establecer si tiene que subir o bajar:



            if(larrow->contains(touch_location))
            {
                right_player->set_speed_x (-player_speed);
            }
            if(rarrow->contains(touch_location))
            {
                right_player->set_speed_x (+player_speed);
            }
            if(tarrow->contains(touch_location))
            {
                right_player->set_speed_y (+player_speed);
            }
            if(barrow->contains(touch_location))
            {
                right_player->set_speed_y (-player_speed);
            }
        }
        else {
            right_player->set_speed_y(0);
            right_player->set_speed_x(0);
        }
    }

    // ---------------------------------------------------------------------------------------------

    void Game_Scene::check_ball_collisions ()
    {



        if (caryellow1 ->intersects (*left_border))
        {
            caryellow1->set_position_x (canvas_width+40);

        }


        if (caryellow2->intersects (*left_border))
        {
            caryellow2->set_position_x (canvas_width+40);

        }




        if (carwhite1 ->intersects (*left_border))
        {
            carwhite1->set_position_x (canvas_width+40);

        }


        if (carwhite2->intersects (*left_border))
        {
            carwhite2->set_position_x (canvas_width+40);

        }




        if (carblue1 ->intersects (*left_border))
        {
            carblue1->set_position_x (canvas_width+40);

        }


        if (carblue2->intersects (*left_border))
        {
            carblue2->set_position_x (canvas_width+40);

        }

        if (bigturtle1->intersects (*left_border))
        {
            bigturtle1->set_position_x (canvas_width+120);

        }

        if (bigturtle2->intersects (*left_border))
        {
            bigturtle2->set_position_x (canvas_width+120);

        }

        if (smallturtle1->intersects (*left_border))
        {
            smallturtle1->set_position_x (canvas_width+80);

        }

        if (smallturtle2->intersects (*left_border))
        {
            smallturtle2->set_position_x (canvas_width+80);

        }



        if (truckmidlane1 ->intersects (*right_border))
        {
            truckmidlane1->set_position_x (-100);

        }




        if (trucklastlane1 ->intersects (*right_border))
        {
            trucklastlane1->set_position_x (-100);

        }

        if (biglog1ll ->intersects (*right_border))
        {
            biglog1ll->set_position_x (-100);

        }

        if (biglog1ml ->intersects (*right_border))
        {
            biglog1ml->set_position_x (-100);

        }

        if (smalllog1 ->intersects (*right_border))
        {
            smalllog1->set_position_x (-100);

        }
        if (smalllog2 ->intersects (*right_border))
        {
            smalllog2->set_position_x (-100);

        }








    }

    // ---------------------------------------------------------------------------------------------

    void Game_Scene::render_loading (Canvas & canvas)
    {
        Texture_2D * loading_texture = textures[ID(loading)].get ();

        if (loading_texture)
        {
            canvas.fill_rectangle
            (
                { canvas_width * .5f, canvas_height * .5f },
                { loading_texture->get_width (), loading_texture->get_height () },
                  loading_texture
            );
        }
    }

    // ---------------------------------------------------------------------------------------------
    // Simplemente se dibujan todos los sprites que conforman la escena.

    void Game_Scene::render_playfield (Canvas & canvas)
    {
        for (auto & sprite : sprites)
        {
            sprite->render (canvas);
        }
    }

    int Game_Scene::option_at (const Point2f & point)
    {
        for (int index = 0; index < number_of_options; ++index)
        {
            const Option & option = options[index];


                return index;

        }

        return -1;
    }


}
