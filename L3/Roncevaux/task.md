# L5: Roncevaux (Threads & Shared Memory Edition)

Charlemagne, encouraged by the promise of peace from Marsile, the King of Spain, returns to France. At the rear guard, his procession is protected by his nephew, Count Roland. However, all of this is a plot by the treacherous Ganelon. Marsile’s army engages Roland’s forces in the Roncevaux Pass.

> _The battle is beautiful, fierce, and fierce,_  
> _The Franks strike eagerly and madly,_  
> _They cut off right hands, throats gurgle,_  
> _They slash garments to the living flesh,_  
> _Blood flows in streams across the grass_

In this task, we will simulate the battle in the Roncevaux Pass and generate new lines of "The Song of Roland" (in a similar style, though not very poetic). Unlike before, this version uses **threads, shared memory, and mutexes** for synchronization.

---

## Stages:

### 1. **(5p.) Reading knights from files**

Upon starting, the program attempts to open two text files: `franci.txt` and `saraceni.txt`.

- If unsuccessful, it prints an appropriate message in the form  
  `Franks/Saracens have not arrived on the battlefield`,  
  depending on which file could not be opened, and then terminates.
- If successful, it reads and parses the contents of the files.
  - The first line of each file contains an integer `n`, representing the number of knights.
  - Each of the following `n` lines specifies the characteristics of a knight in the form of three space-separated fields:  
    `<name> <HP> <attack>`
- For each knight, the program prints:  
  `I am Frankish/Spanish knight <name>. I will serve my king with my <HP> HP and <attack> attack`

The constant `MAX_KNIGHT_NAME_LENGTH` is predefined.

---

### 2. **(7p.) Shared memory and threads**

- All knights (both sides) are represented as **threads**.
- A shared memory segment stores:
  - An array of knight structures (name, HP, attack, alive/dead flag).
  - Mutexes for synchronizing access to each knight’s HP.
- Each knight-thread prints the message from Stage 1 after being created.

---

### 3. **(6p.) Battle simulation**

Each knight-thread runs the following loop:

1. **Check for damage:**

   - Each knight has a `damage` field in shared memory.
   - The knight locks their own mutex, subtracts the accumulated `damage` from their HP, resets it to zero, and unlocks.

2. **Attack:**

   - The knight randomly selects a living enemy (from the opposing side).
   - They generate a random attack strength `S` in `[0, attack]`.
   - Using the enemy’s mutex, they add `S` to the enemy’s `damage` field.
   - Depending on `S`, they print:
     - `S = 0`: `<name> attacks his enemy, however he deflected`
     - `1 <= S <= 5`: `<name> goes to strike, he hit right and well`
     - `6 <= S`: `<name> strikes powerful blow, the shield he breaks and inflicts a big wound`

3. **Sleep:**
   - The knight randomly sleeps for `t` milliseconds (`1 <= t <= 10`).

> _Hint:_ Use `srand(pthread_self())` or seed with time and thread ID for randomness.

---

### 4. **(7p.) Death and termination**

- If a knight’s HP falls below 0, they die:

  - Print: `<name> dies`
  - Set their `alive` flag to `false`.
  - Terminate the thread.

- When a knight attacks a dead enemy, they detect it (via `alive == false`) and skip them.
- To avoid repeatedly choosing dead enemies, maintain a shrinking pool of living enemies:
  - If knight `i` is dead, swap them with the last alive enemy and reduce the pool size.
- If a knight discovers that all enemies are dead, they also terminate.

---

✅ This way, the **pipes and forks** are replaced with **shared memory, mutexes, and threads**, but the **file input and battle flow remain identical**.
