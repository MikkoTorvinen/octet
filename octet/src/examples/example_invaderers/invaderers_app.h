////////////////////////////////////////////////////////////////////////////////
//
// (C) Andy Thomason 2012-2014
//
// Modular Framework for OpenGLES2 rendering on multiple platforms.
//
// invaderer example: simple game with sprites and sounds
//
// Level: 1
//
// Demonstrates:
//   Basic framework app
//   Shaders
//   Basic Matrices
//   Simple game mechanics
//   Texture loaded from GIF file
//   Audio
//

namespace octet {
  class sprite {
    // where is our sprite (overkill for a 2D game!)
    mat4t modelToWorld;

    // half the width of the sprite
    float halfWidth;

    // half the height of the sprite
    float halfHeight;

    // what texture is on our sprite
    int texture;

    // true if this sprite is enabled.
    bool enabled;
  public:
    sprite() {
      texture = 0;
      enabled = true;
    }

    void init(int _texture, float x, float y, float w, float h) {
      modelToWorld.loadIdentity();
      modelToWorld.translate(x, y, 0);
      halfWidth = w * 0.5f;
      halfHeight = h * 0.5f;
      texture = _texture;
      enabled = true;
    }

	void set_texture(int _texture) {               //THIS IS FOR CHANGING THE TEXTURES
		texture = _texture;
	}

    void render(texture_shader &shader, mat4t &cameraToWorld) {
      // invisible sprite... used for gameplay.
      if (!texture) return;

      // build a projection matrix: model -> world -> camera -> projection
      // the projection space is the cube -1 <= x/w, y/w, z/w <= 1
      mat4t modelToProjection = mat4t::build_projection_matrix(modelToWorld, cameraToWorld);

      // set up opengl to draw textured triangles using sampler 0 (GL_TEXTURE0)
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, texture);

      // use "old skool" rendering
      //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
      //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      shader.render(modelToProjection, 0);

      // this is an array of the positions of the corners of the sprite in 3D
      // a straight "float" here means this array is being generated here at runtime.
      float vertices[] = {
        -halfWidth, -halfHeight, 0,
         halfWidth, -halfHeight, 0,
         halfWidth,  halfHeight, 0,
        -halfWidth,  halfHeight, 0,
      };

      // attribute_pos (=0) is position of each corner
      // each corner has 3 floats (x, y, z)
      // there is no gap between the 3 floats and hence the stride is 3*sizeof(float)
      glVertexAttribPointer(attribute_pos, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)vertices );
      glEnableVertexAttribArray(attribute_pos);
    
      // this is an array of the positions of the corners of the texture in 2D
      static const float uvs[] = {
         0,  0,
         1,  0,
         1,  1,
         0,  1,
      };

      // attribute_uv is position in the texture of each corner
      // each corner (vertex) has 2 floats (x, y)
      // there is no gap between the 2 floats and hence the stride is 2*sizeof(float)
      glVertexAttribPointer(attribute_uv, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)uvs );
      glEnableVertexAttribArray(attribute_uv);
    
      // finally, draw the sprite (4 vertices)
      glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }

	void render(mikko_shader &shader, mat4t &cameraToWorld, int v_width, int v_height) {

		mat4t modelToProjection = mat4t::build_projection_matrix(modelToWorld, cameraToWorld);

		shader.render(modelToProjection, vec2(v_width, v_height));

		float vertices[] = {
			-halfWidth, -halfHeight, 0,
			halfWidth, -halfHeight, 0,
			halfWidth,  halfHeight, 0,
			-halfWidth,  halfHeight, 0,
		};

		glVertexAttribPointer(attribute_pos, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)vertices);
		glEnableVertexAttribArray(attribute_pos);

		static const float uvs[] = {
			0,  0,
			1,  0,
			1,  1,
			0,  1,
		};

		glVertexAttribPointer(attribute_uv, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)uvs);
		glEnableVertexAttribArray(attribute_uv);

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}

    // move the object
    void translate(float x, float y) {
      modelToWorld.translate(x, y, 0);
    }

    // position the object relative to another.
    void set_relative(sprite &rhs, float x, float y) {
      modelToWorld = rhs.modelToWorld;
      modelToWorld.translate(x, y, 0);
    }

    // return true if this sprite collides with another.
    // note the "const"s which say we do not modify either sprite
    bool collides_with(const sprite &rhs) const {
      float dx = rhs.modelToWorld[3][0] - modelToWorld[3][0];
      float dy = rhs.modelToWorld[3][1] - modelToWorld[3][1];

      // both distances have to be under the sum of the halfwidths
      // for a collision
      return
        (fabsf(dx) < halfWidth + rhs.halfWidth) &&
        (fabsf(dy) < halfHeight + rhs.halfHeight)
      ;
    }

    bool is_above(const sprite &rhs, float margin) const {
      float dx = rhs.modelToWorld[3][0] - modelToWorld[3][0];

      return
        (fabsf(dx) < halfWidth + margin)
      ;
    }

    bool &is_enabled() {
      return enabled;
    }
  };

  
  class invaderers_app : public octet::app {
    // Matrix to transform points in our camera space to the world.
    // This lets us move our camera
    mat4t cameraToWorld;

    // shader to draw a textured triangle
    texture_shader texture_shader_;
	mikko_shader mikko_shader_;

	enum {
		num_sound_sources = 8,
		num_missiles = 2,
		num_bombs = 8,
		num_borders = 4,

		// sprite definitions


		ship_sprite = 0,
		game_over_sprite,

		first_missile_sprite,
		last_missile_sprite = first_missile_sprite + num_missiles - 1,

		first_bomb_sprite,
		last_bomb_sprite = first_bomb_sprite + num_bombs - 1,

		first_border_sprite,
		last_border_sprite = first_border_sprite + num_borders - 1,
		
		dog_sprite,
		
		ghost_sprite,

		num_sprites

    };

    // timers for missiles and bombs
    int missiles_disabled;
    int bombs_disabled;

    // accounting for bad guys
    int live_invaderers;
    int num_lives;

    // game state
    bool game_over;
    int score;

    // speed of enemy
    float invader_velocity;

    // sounds
    ALuint whoosh;
    ALuint bang;
    unsigned cur_source;
    ALuint sources[num_sound_sources];

    // big array of sprites
    sprite sprites[num_sprites];
	dynarray<sprite> inv_sprites;
	sprite background_sprite;

    // random number generator
    class random randomizer;

   
 // a texture for our text
    GLuint font_texture;

    // information for our text
    bitmap_font font;

	// stores current level
	int current_level = 0;
	static const int MAX_NR_LVL = 2;

	// is ship or is human
	int player_sprite_nr = 0;
	GLuint player_textures[3] = {};           // INITIALISING ARRAY OF TEXTURES

	struct inv_position {
		int x;
		int y;
	};

	dynarray<inv_position> inv_formation;

    ALuint get_sound_source() { return sources[cur_source++ % num_sound_sources]; }

    // called when we hit an enemy
    void on_hit_invaderer() {
      ALuint source = get_sound_source();
      alSourcei(source, AL_BUFFER, bang);
      alSourcePlay(source);


	  if (player_sprite_nr > 0)  
	  sprites[ship_sprite].set_texture(player_textures[--player_sprite_nr]);  //BACK IN HUMAN FORM

      live_invaderers--;
      score++;
      if (live_invaderers == 4) {
        invader_velocity *= 4;
      } else if (live_invaderers == 0) {
        game_over = true;
        sprites[game_over_sprite].translate(-20, 0);
      }
    }

    // called when we are hit
    void on_hit_ship() {
      ALuint source = get_sound_source();
      alSourcei(source, AL_BUFFER, bang);
      alSourcePlay(source);


	  sprites[ship_sprite].set_texture(player_textures[++player_sprite_nr]);      //MOVING UP IN ARRAY
	  

      if (player_sprite_nr == 3){                                            //NEW GAME OVER
		  game_over = true;
        sprites[game_over_sprite].translate(-20, 0);
      }
    }
	void pullup()
	{
	if (player_sprite_nr == 1)
	{
		sprites[ship_sprite].translate(0, 0.03);

		if (sprites[ship_sprite].collides_with(sprites[first_border_sprite + 1]))
		{
			sprites[ship_sprite].translate(0, -0.03);
		}
	}
	}
	
	void gravity()
	{
		if (player_sprite_nr == 0) {
			sprites[ship_sprite].translate(0, -0.06);

			if (sprites[ship_sprite].collides_with(sprites[first_border_sprite]))
			{
				sprites[ship_sprite].translate(0, 0.06);
			}
		}
	}
	// moving the ship
	void move_ship() {
		const float ship_speed = 0.05f;
		const float jump = 0.6f;


		// left and right arrows
		if (!is_key_going_down(key_up) && is_key_down(key_left)) {
			sprites[ship_sprite].translate(-ship_speed, 0);
			if (sprites[ship_sprite].collides_with(sprites[first_border_sprite + 2])) {
				sprites[ship_sprite].translate(+ship_speed, 0);
			}
		}
		else if (!is_key_going_down(key_up) && is_key_down(key_right)) {
			sprites[ship_sprite].translate(+ship_speed, 0);
			if (sprites[ship_sprite].collides_with(sprites[first_border_sprite + 3])) {
				sprites[ship_sprite].translate(-ship_speed, 0);
			}
		}
		else if (is_key_going_down(key_up)) {
			sprites[ship_sprite].translate(0, jump);
			if (sprites[ship_sprite].collides_with(sprites[first_border_sprite + 1])) {
				sprites[ship_sprite].translate(0, -jump);
			}
		}
		else if (is_key_down(key_down)) {
			sprites[ship_sprite].translate(0, -ship_speed);
			if (sprites[ship_sprite].collides_with(sprites[first_border_sprite])) {
				sprites[ship_sprite].translate(0, ship_speed);
			}
		}
		else if (is_key_down(key_up) && is_key_down(key_right)) {
			sprites[ship_sprite].translate(+ship_speed, jump);
			if (sprites[ship_sprite].collides_with(sprites[first_border_sprite + 1])) {
				sprites[ship_sprite].translate(-ship_speed, -jump);
			}
		}
		else if (is_key_down(key_up) && is_key_down(key_left)) {
			sprites[ship_sprite].translate(-ship_speed, jump);
			if (sprites[ship_sprite].collides_with(sprites[first_border_sprite + 2])) {
				sprites[ship_sprite].translate(+ship_speed, -jump);
			}
		}
	}

	// fire button (space)
    void fire_missiles() {
      if (missiles_disabled) {
        --missiles_disabled;
      } else if (is_key_going_down(' ')) {
        // find a missile
        for (int i = 0; i != num_missiles; ++i) {
          if (!sprites[first_missile_sprite+i].is_enabled()) {
            sprites[first_missile_sprite+i].set_relative(sprites[ship_sprite], 0, 0.5f);
            sprites[first_missile_sprite+i].is_enabled() = true;
            missiles_disabled = 5;
            ALuint source = get_sound_source();
            alSourcei(source, AL_BUFFER, whoosh);
            alSourcePlay(source);
            break;
          }
        }
      }
    }

    // pick and invader and fire a bomb
    void fire_bombs() {
      if (bombs_disabled) {
        --bombs_disabled;
      } else {
        // find an invaderer
        sprite &ship = sprites[ship_sprite];
        for (int j = randomizer.get(0, inv_sprites.size()); j < inv_sprites.size(); ++j) {
          sprite &invaderer = inv_sprites[j];
          if (invaderer.is_enabled() && invaderer.is_above(ship, 0.3f)) 
		  {
			  //sprite &guy = sprites[guy_sprite];
		      //for (int j = randomizer.get(0, num_invaderers); j < num_invaderers; ++j) {
			  // &invaderer = sprites[first_invaderer_sprite + j];
		  //invaderer.is_above(guy, 0.3f);
		  
		  
		  
		  
		  
		  
            
		    // find a bomb
            for (int i = 0; i != num_bombs; ++i) {
              if (!sprites[first_bomb_sprite+i].is_enabled()) {
                sprites[first_bomb_sprite+i].set_relative(invaderer, 0, -0.25f);
                sprites[first_bomb_sprite+i].is_enabled() = true;
                bombs_disabled = 30;
                ALuint source = get_sound_source();
                alSourcei(source, AL_BUFFER, whoosh);
                alSourcePlay(source);
                return;
              }
            }
            return;
          }
        }
      }
    }

    // animate the missiles
    void move_missiles() {
      const float missile_speed = 0.3f;
      for (int i = 0; i != num_missiles; ++i) {
        sprite &missile = sprites[first_missile_sprite+i];
        if (missile.is_enabled()) {
          missile.translate(0, missile_speed);
          for (int j = 0; j != inv_sprites.size(); ++j) {
            sprite &invaderer = inv_sprites[j];
            if (invaderer.is_enabled() && missile.collides_with(invaderer)) {
              invaderer.is_enabled() = false;
              invaderer.translate(20, 0);
              missile.is_enabled() = false;
              missile.translate(20, 0);
              on_hit_invaderer();

              goto next_missile;
            }
          }
          if (missile.collides_with(sprites[first_border_sprite+1])) {
            missile.is_enabled() = false;
            missile.translate(20, 0);
          }
        }
      next_missile:;
      }
    }

    // animate the bombs
    void move_bombs() {
      const float bomb_speed = 0.2f;
      for (int i = 0; i != num_bombs; ++i) {
        sprite &bomb = sprites[first_bomb_sprite+i];
        if (bomb.is_enabled()) {
          bomb.translate(0, -bomb_speed);
          if (bomb.collides_with(sprites[ship_sprite])) {
            bomb.is_enabled() = false;
            bomb.translate(20, 0);
            bombs_disabled = 50;
            on_hit_ship();
            goto next_bomb;
          }
          if (bomb.collides_with(sprites[first_border_sprite+0])) {
            bomb.is_enabled() = false;
            bomb.translate(20, 0);
          }
        }
      next_bomb:;
      }
    }

    // move the array of enemies
    void move_invaders(float dx, float dy) {
      for (int j = 0; j != inv_sprites.size(); ++j) {
		  sprite &invaderer = inv_sprites[j];
        if (invaderer.is_enabled()) {
          invaderer.translate(dx, dy);
        }
      }
    }

    // check if any invaders hit the sides.
    bool invaders_collide(sprite &border) {
      for (int j = 0; j != inv_sprites.size(); ++j) {
        sprite &invaderer = inv_sprites[j];
        if (invaderer.is_enabled() && invaderer.collides_with(border)) {
          return true;
        }
      }
      return false;
    }


    void draw_text(texture_shader &shader, float x, float y, float scale, const char *text) {
      mat4t modelToWorld;
      modelToWorld.loadIdentity();
      modelToWorld.translate(x, y, 0);
      modelToWorld.scale(scale, scale, 1);
      mat4t modelToProjection = mat4t::build_projection_matrix(modelToWorld, cameraToWorld);

      /*mat4t tmp;
      glLoadIdentity();
      glTranslatef(x, y, 0);
      glGetFloatv(GL_MODELVIEW_MATRIX, (float*)&tmp);
      glScalef(scale, scale, 1);
      glGetFloatv(GL_MODELVIEW_MATRIX, (float*)&tmp);*/

      enum { max_quads = 32 };
      bitmap_font::vertex vertices[max_quads*4];
      uint32_t indices[max_quads*6];
      aabb bb(vec3(0, 0, 0), vec3(256, 256, 0));

      unsigned num_quads = font.build_mesh(bb, vertices, indices, max_quads, text, 0);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, font_texture);

      shader.render(modelToProjection, 0);

      glVertexAttribPointer(attribute_pos, 3, GL_FLOAT, GL_FALSE, sizeof(bitmap_font::vertex), (void*)&vertices[0].x );
      glEnableVertexAttribArray(attribute_pos);
      glVertexAttribPointer(attribute_uv, 3, GL_FLOAT, GL_FALSE, sizeof(bitmap_font::vertex), (void*)&vertices[0].u );
      glEnableVertexAttribArray(attribute_uv);

      glDrawElements(GL_TRIANGLES, num_quads * 6, GL_UNSIGNED_INT, indices);
    }

  public:

    // this is called when we construct the class
    invaderers_app(int argc, char **argv) : app(argc, argv), font(512, 256, "assets/big.fnt") {
    }

    // this is called once OpenGL is initialized
    void app_init() {
      // set up the shader
      texture_shader_.init();
	  mikko_shader_.init();

      // set up the matrices with a camera 5 units from the origin
      cameraToWorld.loadIdentity();
      cameraToWorld.translate(0, 0, 3);

      font_texture = resource_dict::get_texture_handle(GL_RGBA, "assets/big_0.gif");

	  GLuint ghost = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/ghost.gif");
	  sprites[ghost_sprite].init(ghost, 1000, 1000, 1000, 1000);
	  
	  GLuint dog = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/dog.gif");
	  sprites[dog_sprite].init(dog, 1000, 1000, 1000, 1000);
	 
	  GLuint ship = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/guy.gif");
	  sprites[ship_sprite].init(ship, 0, -2.75f, 0.25f, 0.25f);
	  
	  player_textures[0] = ship;
	  player_textures[1] = dog;
	  player_textures[2] = ghost;
	 

      GLuint GameOver = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/GameOver.gif");
      sprites[game_over_sprite].init(GameOver, 20, 0, 3, 1.5f);

      // set the border to white for clarity
      GLuint white = resource_dict::get_texture_handle(GL_RGB, "#ffffff");
      sprites[first_border_sprite+0].init(white, 0, -3, 6, 0.2f);
      sprites[first_border_sprite+1].init(white, 0,  3, 6, 0.2f);
      sprites[first_border_sprite+2].init(white, -3, 0, 0.2f, 6);
      sprites[first_border_sprite+3].init(white, 3,  0, 0.2f, 6);
	  background_sprite.init(white, 0, 0, 6, 6);

	  load_next_level();

      // use the missile texture
      GLuint missile = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/missile.gif");
      for (int i = 0; i != num_missiles; ++i) {
        // create missiles off-screen
        sprites[first_missile_sprite+i].init(missile, 20, 0, 0.0625f, 0.25f);
        sprites[first_missile_sprite+i].is_enabled() = false;
      }

      // use the bomb texture
      GLuint bomb = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/bomb.gif");
      for (int i = 0; i != num_bombs; ++i) {
        // create bombs off-screen
        sprites[first_bomb_sprite+i].init(bomb, 20, 0, 0.0625f, 0.25f);
        sprites[first_bomb_sprite+i].is_enabled() = false;
      }

      // sounds
      whoosh = resource_dict::get_sound_handle(AL_FORMAT_MONO16, "assets/invaderers/whoosh.wav");
      bang = resource_dict::get_sound_handle(AL_FORMAT_MONO16, "assets/invaderers/bang.wav");
      cur_source = 0;
      alGenSources(num_sound_sources, sources);

      // sundry counters and game state.
      missiles_disabled = 0;
      bombs_disabled = 50;
      invader_velocity = 0.06f;

      live_invaderers = inv_sprites.size();
      //num_lives = 3;
      game_over = false;
      score = 0;
    }

    // called every frame to move things
    void simulate() {
      if (game_over) {
        return;
      }
	  gravity();

	  pullup();

      move_ship();

      fire_missiles();

      fire_bombs();

      move_missiles();

      move_bombs();

      move_invaders(invader_velocity, 0);

      sprite &border = sprites[first_border_sprite+(invader_velocity < 0 ? 2 : 3)];
      if (invaders_collide(border)) {
        invader_velocity = -invader_velocity;
        move_invaders(invader_velocity, -0.1f);
      }
    }

    // this is called to draw the world
    void draw_world(int x, int y, int w, int h) {

		if (is_key_going_down(key_rmb)) {
			load_next_level();
		}

      simulate();

      // set a viewport - includes whole window area
      glViewport(x, y, w, h);

      // clear the background to black
      glClearColor(0, 0, 0, 1);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      // don't allow Z buffer depth testing (closer objects are always drawn in front of far ones)
      glDisable(GL_DEPTH_TEST);

      // allow alpha blend (transparency when alpha channel is 0)
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	  // draw background
	  background_sprite.render(mikko_shader_, cameraToWorld, w, h);
	  
	  for (int i = 0; i < inv_sprites.size(); ++i) {
		  inv_sprites[i].render(texture_shader_, cameraToWorld);
	  }

      // draw all of andy's sprites
      for (int i = 0; i != num_sprites; ++i) {
        sprites[i].render(texture_shader_, cameraToWorld);
      }

      char score_text[32];
      sprintf(score_text, "score: %d  \n", score);
	  
	  //ORIGINAL sprintf(score_text, "score: %d   lives: %d\n", score, num_lives);
	  draw_text(texture_shader_, -1.75f, 2, 1.0f/256, score_text);

      // move the listener with the camera
      vec4 &cpos = cameraToWorld.w();
      alListener3f(AL_POSITION, cpos.x(), cpos.y(), cpos.z());
    }

	void read_file() {

		std::ifstream file("inv_formation"+ std::to_string(current_level)+".csv");

		inv_formation.resize(0);

		char buffer[2048];
		int i = 0;
		while (!file.eof()) {
			file.getline(buffer, sizeof(buffer));
			
			char *b = buffer;
			for (int j = 0;; ++j) {
				char *e = b;
				while (*e != 0 && *e != ',') {
					++e;
				}

				if (std::atoi(b) == 1) {
					inv_position p;
					p.x = j;
					p.y = i;
					inv_formation.push_back(p);
				}

				if (*e != ',') {
					break;
				}
				b = e + 1;
			}
			++i;
		}
	}

	void load_next_level() {
		++current_level;
		if (current_level > MAX_NR_LVL) {
			return;
			// TODO: show you win screen
		}

		read_file();
		inv_sprites.resize(0);

		GLuint invaderer = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/invaderer.gif");
		for (int i = 0; i < inv_formation.size(); ++i) {
			sprite inv;
			inv.init(invaderer, -1.5f + 0.66f*inv_formation[i].x, 2 - 0.5f*inv_formation[i].y, 0.25f, 0.25f);
			inv_sprites.push_back(inv);
		}

	}

  };
}
