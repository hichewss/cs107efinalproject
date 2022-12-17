# CS107E Final Project
## Project title
Aim Labs
## Team members
Felicity Huang, Vincent Thai, Chuyi Zhang

## Project description
We created the CS107e version of the popular aim training game Aimlab. Users will load into a start screen until they hit the trigger button on the nunchuck. They will then have one minute to hit as many targets as possible. The game will calculate the user's accuracy and score and display them in real time. After the game ends, the user will be able to see their results in a design modeled off of Aimlabs' performance statistics UI. If the user makes the top 10, they will be prompted to enter their username to be saved onto the leaderboard. All users will be shown the leaderboard briefly before returning to the start screen
.
## Member contribution
A short description of the work performed by each member of the team.
Chuyi - I relied heavily on console.h to create the UI framework for the game. Using math logic to determine the dimensions and placement of the display's text, I developed a generalizable start screen, end screen, and leaderboard screen, the latter of which updates whenever a new user plays the game and/or is knocked off the top 10. I also supported my teammates with minor debugging issues and created the trailer played during our demo :D

Felicity - Created main point and click shooting game component. The graphics were created with gl.h. Spawned targets in random positions with rand.h provided in cs107e src. Implemented the game HUD ui to closely resemble that of Aimlab. The HUD bar on top tracks and updates the display of the score, timer countdown, and accuracy of the player in real time. Made sure elements can be easily adjusted like size of the targets and tried to make elements like the spawn area of the targets and the UI generalizable to different screen sizes by not hard-coding things. When the game ends, it returns a game data struct for the stats screen. I configured ps2 mouse to work with the game first (mouseAimlab.c) and then updated the code to work with nunchuck inputs (nunchuckAimlab.c). 

Vincent - Implementing nunchuck controller, meaning tuning the i2c, datasheets for communication and reading/processing
data, and then cleaning the data to get a decent sensitivity. Also bugfixes with the other parts of the program here and there.

## References

Lots of looking at youtube and github as well as homebrew datasheets
 to figure out what do to initialize/communicate with the controller and then translating the code into
 something workable for the scope of this game. I tinkered with Logic analyzer to experiement/modify 
two i2c libraries (the 107e reference and lixiaofu@stanford.edu's personal library) and eventually got the
 reference version to work by editing the timing of the i2c functions. Also helped debug the leaderboard
 and the updating of the screen during the actual game, and worked to calibrate the nunchuck's sensitivity to
 something playable. Tried tinkering with audio library to add some SFX but ran out of time.
 
Vincent - Lots of homebrew websites/datasheets detailing how people hacked the nintendo controller, including mainly links from Adafuit's website and youtube videos with their own repos detailing what they did. All of the nunchuck code itself was written by me though.
I used the reference libraries for i2c, with a small modification to stretch the i2c write function so that it would work with the nunchuck. I also pulled into this project folder/repo some audio libraries for tinkering but none of that work made it into the demo version.
Felicity - Rand.h, https://www.youtube.com/watch?v=HoDBRRVuVQQ&ab_channel=shaman for the gameplay and design.
Chuyi - I used the score statistics page from Aimlab as reference to build the results screen for the game. The png is attached with this submission file. It's called "ui". 

Felicity -  

## Self-evaluation
How well was your team able to execute on the plan in your proposal?  
Any trying or heroic moments you would like to share? Of what are you particularly proud: the effort you put into it? the end product? 
the process you followed? what you learned along the way? Tell us about it!

We think we worked together and communicated really well, and we got everything that we wanted to get done accomplished by demo day. Even though we had a backup plan in case the nunchuck didn't work out, we're really proud of the fact that we didn't have to rely on it in the end and that everything came together nicely. We're also proud of how much effort weput into creating and polishing the project and for the time spent in Gates working on our code. 

If anything, we learned not to procrastinate, haha. We planned way ahead in advance and made sure to support each other throughout the process. We also learned not to be too ambitious with our idea, especially given the time constraint. 

## Photos
You are encouraged to submit photos/videos of your project in action. 
Add the files and commit to your project repository to include along with your submission.

We made a trailer of our final project and us developing the project! It's too big to attach, but you can see it in the replay of our demo :D 
>>>>>>> e6635459d38336aef049d9af68b1123b5688a64b
