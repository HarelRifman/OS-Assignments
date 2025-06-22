#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

// Signal handlers
void email_handler(int sig) {}
void delivery_handler(int sig) {}
void doorbell_handler(int sig) {}

void setupSignalHandlers() {
    struct sigaction sa;
    
    // Setup SIGUSR1 for email notifications
    sa.sa_handler = email_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);
    
    // Setup SIGUSR2 for delivery reminders
    sa.sa_handler = delivery_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR2, &sa, NULL);
    
    // Setup SIGTERM for doorbell
    sa.sa_handler = doorbell_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);
}

void printFocusModeHeader(int round_number, int duration) {
    char input[3];
    int choice;
    sigset_t block_set, pending_set;
    
    // Block all our signals during focus round
    sigemptyset(&block_set);
    sigaddset(&block_set, SIGUSR1);
    sigaddset(&block_set, SIGUSR2);
    sigaddset(&block_set, SIGTERM);
    sigprocmask(SIG_BLOCK, &block_set, NULL);
    
    printf("══════════════════════════════════════════════\n");
    printf("                Focus Round %d                \n", round_number);
    printf("──────────────────────────────────────────────\n");
    
    for (int i = 0; i < duration; i++) {
        printf("\nSimulate a distraction:\n");
        printf("  1 = Email notification\n");
        printf("  2 = Reminder to pick up delivery\n");
        printf("  3 = Doorbell Ringing\n");
        printf("  q = Quit\n");
        printf(">> ");
        
        if (scanf("%s", input) == 1) {
            if (strcmp(input, "q") == 0) {
                break;
            }
            choice = atoi(input);
            
            // Send appropriate signal based on choice
            switch (choice) {
                case 1:
                    kill(getpid(), SIGUSR1);
                    break;
                case 2:
                    kill(getpid(), SIGUSR2);
                    break;
                case 3:
                    kill(getpid(), SIGTERM);
                    break;
            }
        }
    }
    
    printf("──────────────────────────────────────────────\n");  // This will appear right after >>
    printf("        Checking pending distractions...      \n");
    printf("──────────────────────────────────────────────\n");
    
    // Check for pending signals
    sigpending(&pending_set);
    
    int any_pending = 0;
    
    // Check each signal in order (1, 2, 3)
    if (sigismember(&pending_set, SIGUSR1)) {
        printf(" - Email notification is waiting.\n");
        printf("[Outcome:] The TA announced: Everyone get 100 on the exercise!\n");
        any_pending = 1;
    }
    
    if (sigismember(&pending_set, SIGUSR2)) {
        printf(" - You have a reminder to pick up your delivery.\n");
        printf("[Outcome:] You picked it up just in time.\n");
        any_pending = 1;
    }
    
    if (sigismember(&pending_set, SIGTERM)) {
        printf(" - The doorbell is ringing.\n");
        printf("[Outcome:] Food delivery is here.\n");
        any_pending = 1;
    }
    
    if (!any_pending) {
        printf("No distractions reached you this round.\n");
    }
    
    printf("──────────────────────────────────────────────\n");
    printf("             Back to Focus Mode.              \n");
    printf("══════════════════════════════════════════════\n");
    
    // Unblock signals to handle them
    sigprocmask(SIG_UNBLOCK, &block_set, NULL);
    
    // Brief pause to allow signal handlers to run
    usleep(1000);
}

void runFocusMode(int numOfRounds, int duration) {
    printf("Entering Focus Mode. All distractions are blocked.\n");
    
    setupSignalHandlers();
    
    for (int i = 0; i < numOfRounds; i++) {
        printFocusModeHeader(i + 1, duration);
    }
    
    printf("\nFocus Mode complete. All distractions are now unblocked.");
}