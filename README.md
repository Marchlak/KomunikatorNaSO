# Running the Program
After compiling the program using the provided makefile, you need to start the server by executing:

## bash
Copy code
./komunikator -s
From this point, the server listens for user logins and actions. Note that you cannot run two servers simultaneously. After launching the server, you can log in a user in a different console with:

## bash
Copy code
./komunikator -l username
You can also add the -d option before the -l option to enable file sending to us, for example:

## bash
Copy code
./komunikator -d "path" -l nickname
Testing the Project
Testing involves observing system logs and messages printed to standard output and standard error. These logs will inform whether an action was successful or not. Not all messages are displayed by default. To facilitate debugging, you can enter the directive:


### Implemented Features
- Message Exchange: Allows users to send messages to each other.
- File Copying: Enables copying files between users.
- User Login: Supports user login operations.
- User Logout: Users can log out using Ctrl+C.
- Logging: Important actions are sent to system logs, some actions are printed to standard error, and errors that do not terminate the program are printed to standard output.
- Server Shutdown Handling: Shuts down every client in case the server is stopped.
- Active User Display: Prints a list of all active users.
  
points 34/34
