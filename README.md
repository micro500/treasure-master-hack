![http://upload.wikimedia.org/wikipedia/en/thumb/3/3a/TreasureMasterNes.jpg/220px-TreasureMasterNes.jpg](http://upload.wikimedia.org/wikipedia/en/thumb/3/3a/TreasureMasterNes.jpg/220px-TreasureMasterNes.jpg)

Treasure Master is a game for the NES in 1991. The game was made for an MTV contest which ran in April 1992. The game has 5 levels which the player can access without a special code. On April 11, 1992 a special code was released which unlocked a 6th level. Players would need to restart the game, enter the code, then play through the first 5 levels again to get to the new level. Upon beating the new level players won a prize.

The developers claimed the security in the game was nearly unbreakable, but the people of Reddit decided to give a shot anyway.

http://blog.reddit.com/2009/09/help-reddit-hack-worlds-worst-nintendo.html

As a result of that hackathon, some people (myself included) got in touch with the developers who told us about a second prize world that has yet to be found.

Through some memory manipulation I managed to [see this new level](http://www.reddit.com/r/TreasureMaster/comments/ba877/second_prize_world_map/), but it was unplayable. So I started looking into how the code is checked to open this world. What I found is that there is no easy way to find the code other than brute forcing it.

Written in C++ (currently), this utility will process each possible code and note if any possibly could be correct.

In the future I want to expand this to use CUDA as well as port it to an FPGA.