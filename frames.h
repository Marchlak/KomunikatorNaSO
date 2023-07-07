#ifndef FRAMES_H
#define FRAMES_H

int frame_splitting_command(char *source, char *frame_command);
int frame_splitting_command_sender(char *source, char *frame_command, char *frame_sender);
int frame_splitting_command_receiver_message(char *source, char *frame_command, char *frame_receiver, char *frame_message);
int frame_splitting_command_sender_receiver_message(char *source, char *frame_command, char *frame_sender, char *frame_receiver, char *frame_message);


#endif
