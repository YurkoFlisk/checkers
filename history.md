# HISTORY 
## VERSIONS
###0.6 
* basic version

###0.7 
* added ability of playing on two sides for user(with board reversing) and restarting

###0.8 
* some graphics bugs fixed
* AI improved by applying alpha-beta pruning optimization to minimax algorithm

###0.9 
* added basic computer move animation
* added ability of manual board reversing(menu item)
* added ability of playing pvp games
* AI search depth increased

###1.0 
* AI computations are moved to a separate thread, so they are not blocking GUI now

###1.1 
* added ability of undoing moves

###1.2 
* fixed various bugs
* improved AI by adding ability to remember scores for some of computed positions and to use them later(transposition tables)
* added ability of redoing undone moves
* added ability of requesting hints from ai
* added ability of choosing AI search depth

###1.3 
* fixed rules bug(throughout the move captured pieces should not disappear from the board, and they should not be captured more than once)
* GUI modified for fixed rules
* added turn indicator at right-upper corner
* added ability of saving and loading played games
* last played game is automatically saved to last_game.txt file in program directory
* improved AI by adding killer heuristic and increased max search depth to 12
* added detection of draw(position repeated 3 times)

###1.4 
* positions in moves in game files now can be separated by ':' symbol
* optimized engine core by applying move semantics where possible
* improved score function for AI
* improved AI by adding quiescence search when evaluating position(now score is computed with score() function only when no capture-moves available)
* improved AI by adding history heuristic
* improved GUI. Now all possible moves are displayed for currently selected piece
* started using GDI+ for GUI, added smoothing, optimized graphics
* added ability of playing misere games
