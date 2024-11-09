#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define WORD_LENGTH 5
#define MAX_ATTEMPTS 6
#define MAX_WORDS 5000

// Arrays to store the words from the files
char valid_solutions[MAX_WORDS][WORD_LENGTH + 1];
char valid_guesses[MAX_WORDS][WORD_LENGTH + 1];
int solution_count = 0;
int guess_count = 0;

// Function to load words from a file into an array
int load_words(const char *filename, char words[][WORD_LENGTH + 1]) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }

    int count = 0;
    while (fscanf(file, "%5s", words[count]) == 1) {
        count++;
        if (count >= MAX_WORDS) break;  // Prevent buffer overflow
    }

    fclose(file);
    return count;
}

// Function to provide feedback for the guess
void get_feedback(const char *guess, const char *target, char *feedback) {
    for (int i = 0; i < WORD_LENGTH; i++) {
        if (guess[i] == target[i]) {
            feedback[i] = 'G';  // Green for correct position
        } else if (strchr(target, guess[i]) != NULL) {
            feedback[i] = 'Y';  // Yellow for correct letter in wrong position
        } else {
            feedback[i] = 'B';  // Black for incorrect letter
        }
    }
    feedback[WORD_LENGTH] = '\0'; // Null-terminate feedback string
}

// Function to validate if the guessed word exists in the valid guesses list
int is_valid_guess(const char *guess) {
    for (int i = 0; i < guess_count; i++) {
        if (strcmp(guess, valid_guesses[i]) == 0) {
            return 1;
        }
    }
    for (int i = 0; i < solution_count; i++){
        if (strcmp(guess, valid_solutions[i] == 0)) return 1;
    }
    return 0;
}

// Function to play a turn for a player and return points earned
int player_turn(const char *player_name, const char *target_word) {
    char guess[WORD_LENGTH + 1];
    char feedback[WORD_LENGTH + 1];

    printf("%s's turn: ", player_name);
    scanf("%5s", guess);

    // Validate guess length
    if (strlen(guess) != WORD_LENGTH) {
        printf("Please enter a %d-letter word.\n", WORD_LENGTH);
        return -1; // Invalid input, no points earned, and retry needed
    }

    // Validate if guess exists in the word list
    if (!is_valid_guess(guess)) {
        printf("Please enter an existing %d-letter word.\n", WORD_LENGTH);
        return -1; // Invalid input, no points earned, and retry needed
    }

    // Provide feedback
    get_feedback(guess, target_word, feedback);
    printf("Feedback: %s\n", feedback);

    // Check if the guess matches the target word
    if (strcmp(guess, target_word) == 0) {
        printf("Congratulations %s! You've guessed the word!\n", player_name);
        return 1; // Earn points for a correct guess
    }

    return 0; // No points if the guess is incorrect
}

// Main game function
void wordle_head_to_head() {
    srand(time(NULL));
    const char *target_word = valid_solutions[rand() % solution_count];

    printf("Welcome to Head-to-Head Wordle!\n");
    printf("Two players take turns to guess the same word.\n");

    int player1_score = 0;
    int player2_score = 0;
    const char *players[2] = {"Player 1", "Player 2"};
    int current_player = 0; // Start with Player 1

    for (int round = 1; round <= MAX_ATTEMPTS; round++) {
        for (int i = 0; i < 2; i++) { // Each player has one turn per round
            const char *current_player_name = players[current_player];
            printf("\nRound %d: %s's turn.\n", round, current_player_name);

            int result;
            do {
                result = player_turn(current_player_name, target_word);
            } while (result == -1); // Repeat if input was invalid

            // If guess is correct, end the game
            if (result == 1) {
                printf("%s wins with %d points!\n", current_player_name, MAX_ATTEMPTS - round + 1);
                return;
            }

            // Switch to the next player
            current_player = 1 - current_player;
        }
    }

    // If no player guesses correctly within MAX_ATTEMPTS rounds
    printf("\nBoth players have used all attempts. The word was: %s\n", target_word);
    printf("It's a draw!\n");
}

int main() {
    // Load words from files
    solution_count = load_words("valid_solutions.txt", valid_solutions);
    guess_count = load_words("valid_guesses.txt", valid_guesses);

    if (solution_count == -1 || guess_count == -1) {
        printf("Failed to load word lists.\n");
        return 1;
    }

    printf("Loaded %d solutions and %d guesses.\n", solution_count, guess_count);

    // Start the game
    wordle_head_to_head();

    return 0;
}
