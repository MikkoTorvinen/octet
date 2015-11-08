CANDY CRUSHER DIPTYCH

First, we make changes to player movements by adding options for upward and downward movement. The character’s movements are controlled by the following script. 

if (is_key_going_down(key_up)) {
    
sprites[ship_sprite].translate(0, jump);
	
if (sprites[ship_sprite].collides_with(sprites[first_border_sprite + 1])) {
	   
 sprites[ship_sprite].translate(0, -jump);

The movement speed is determined by “void move_ship” function. For the collision part we have to add a negative ship speed. Once the ship_sprite will hit the border sprite the negative translation value is zeroing out the movement speed, so the character won't walk out from the scene.
Jump function is just a faster translation value in simulation time.  For making that work in game, we need to create a force that will pull the character back to the “ground”.
We add gravity by affecting the character sprite with continuous negative translate value in y-axis. When character sprite will hit the bottom border we will have opposite force for affecting player sprite, so it will zero out the gravity effect.
We need to add gravity in our simulation as well, to call the function in every frame so it will have an effect in game.

Next, we are creating a new character. The goal is to have a different character each time the player will be hit by the enemy and will be closer to a death and game over. Player has a change to transform back to full health, by shooting the enemies.
First, we load sprites into our program and translate them away from our scene.
	
GLuint ghost = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/ghost.gif");
	  
sprites[ghost_sprite].init(ghost, 1000, 1000, 1000, 1000);

Then we set up them in our texture array. In this case ghost will be last in array
	
player_textures[2] = ghost;

We need to declare new sprites in our sprite definitions by adding a line in our enum function
	
ghost_sprite,

We initialize array of textures by
	
int player_sprite_nr = 0

GLuint player_textures[3] = {};

For changing the textures we write the following:
	
void set_texture(int _texture) {
        	
texture = _texture; }


For character texture changes to take effect we write the following line in on_hit_ship function.
	sprites[ship_sprite].set_texture(player_textures[++player_sprite_nr]);
This function will be called every time we are hit by enemy and it will move one up in our array of textures.
For player to get back to full health and move down in the array, we need to write a new statement in our function, which is called when we hit an enemy. We can’t go below the zero value, so we set it up with if statement. Zero value is when we are in full health.
	if (player_sprite_nr > 0)  
	sprites[ship_sprite].set_texture(player_textures[--player_sprite_nr]);
Since we only have one life, we have to change the game over function according to that. First we delete the old if statement in our “on_hit_ship” function. 
The new if statement for game over will be called when player sprite number will reach one value over the existing player forms, which in our case is the ghost, which has a value of 2.
	(player_sprite_nr == 3)

We use CSV (comma separated values) file to position enemies, and also add one more level into our scene. Since we are using CSV file we need to clear some of the code that we are going to initialize in our CSV function.

From enum we don’t need number of rows and number columns, because we are using data from aour CSC file that is dynamic.
(Number of bombs need to go as well, because (no idea))
Number of invaderers is no longer determined by rows times columns, because we are using the CSV file for that purpose.

Same for these
	 invader_sprite
	first_invader_sprite
	last invader_sprite = first_invader_sprite + num_invadereres - 1



We had to add dynamic array for storing invaders from CSV file. Since we don’t know how many invadederes there will be.
	dynarray<sprite> inv_sprites;
	live_invaderers = inv_sprites.size();
	// draw all the sprites
      	for (int i = 0; i < inv_sprites.size(); ++i) {
         	 inv_sprites[i].render(texture_shader_, cameraToWorld); }



In our CSV function first we set the path where the CSV files are located. Since we have multiple files we are using a string command.
	std::ifstream file("inv_formation"+ std::to_string(current_level)+".csv");
We need to store it in the buffer
	char buffer[2048];
For reading the CSV file we are using double loops. One for looping over lines.
	while (!file.eof()) {
	file.getline(buffer, sizeof(buffer));

And the other one looping over columns.
	char *b = buffer;
	for (int j = 0;; ++j) {
	char *e = b;
	while (*e != 0 && *e != ',') {
	++e;}

At this point b stores all the characeters for an element of the csv file, but we have to transformed into a number. We are suing atoi method to convert the char array to an int, further we are comparing the resulting number to see if it is the value 1 (which we have designated as being the identifier for creating an invaderer). If this is the case, then we store the position (column and row) of the element in the csv file in the inv_formation array as the x and y coordinates of the invaderer.
	if (std::atoi(b) == 1) {
	inv_position p;
	p.x = j;
	p.y = i;
	inv_formation.push_back(p); }

CSV Level
We create a function for loading next level by moving up to our next CSV file.
	void load_next_level() {
		++current_level;
This function will be called when there will be no enemies left in our scene. We change the previous function when enemies are shot, from game over to load next level.
	if (live_invaderers == 0) {
			load_next_level(); }





SHADER
Shader is a program that computes a color value of a pixel. Shader is a compilation of vertex and fragment programs. These programs are the two largest parts of the rendering pipeline.
Primitives refers in our case to a stream of vertices which can be sequenced into points, lines, or triangles. 
Rasterization consists of two parts. First determines which squares of an integer grid are occupied by these primitives. Second part assigns a color and a depth value to each square in the grid.
With draw function openGL sends vertex locations to vertex shader. The main function for vertex shader is to output vertex positions on screen space. Depending on vertex positions and connections the screen space will be fragmented. These fragments, which are interpolated by rasterizer are then passed to the fragment shader. Any custom data that is interpolated in vertex shader will be sent to fragment shader to execute as well. Fragment shader will output the RGBA color values.
In our shader following lines are for the vertex shader.
	const char vertex_shader[] = SHADER_STR(
	attribute vec4 pos;
	uniform mat4 modelToProjection;
	void main() { 
	gl_Position = modelToProjection * pos; } );

Fragment shader is following.
	const char fragment_shader[] = SHADER_STR(
	uniform vec2 resolution;
	void main() { 
	vec2 pos = gl_FragCoord.xy / resolution.xy;
	vec3 vColor = vec3(0.6, 0.1, 0.3) * (1-pos.y);
	gl_FragColor = vec4(vColor, 1.0); } );
	
I created a sprite for the background which is coloured with a custom shader. The vertex shader part is just passing through, not modifying the transformation of the vertices, but the fragment shader makes use of the y coordinate of the current pixel to create a gradient effect in the colouring. This is done by multiplying the RGB values of the final colour of the pixel by the normalized y value. We have to normalize the y axis values because colours for the fragment shader are in the range [0, 1].
