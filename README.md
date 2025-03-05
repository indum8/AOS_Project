# Trusted Daemon Project

This project implements a trusted daemon server that authenticates clients using UNIX domain sockets and an SQLite database. The server verifies client credentials and shares file access if the authentication is successful. The project uses event notification (`select`) to manage concurrency and passes file descriptors between the server and the client.

## Features

- Uses UNIX domain sockets for communication with a known path for the name of the socket.
- Uses `accept` to create a connection-per-client.
- Uses `select` to manage concurrency.
- Uses domain socket facilities to get a trustworthy identity of the client (i.e., user ID).
- Passes file descriptors between the service and the client.
- Hosts a database (SQLite) and provides its own policies for allowing clients to access different parts of the database.

## Requirements

- GCC (GNU Compiler Collection)
- SQLite3 library
- UNIX-based operating system (e.g., macOS, Linux)

## Setup

1. **Clone the repository:**
   ```sh
   git clone https://github.com/yourusername/trusted_daemon.git
   cd trusted_daemon

2. **Create the SQLite database and users table:**
    ```sh
   sqlite3 users.db "CREATE TABLE users (username TEXT, password TEXT);"
   sqlite3 users.db "INSERT INTO users (username, password) VALUES ('user1', 'password1');"


3. **Create the permissions table:**
    ```sh
   sqlite3 users.db "CREATE TABLE permissions (username TEXT, resource TEXT);"
   sqlite3 users.db "INSERT INTO permissions (username, resource) VALUES ('user1', 'shared_file.txt');"

4. **Create the shared file:**
    ```sh
   echo "This is a shared file." > shared_file.txt

5. **Compile the server and client:**
   ```sh
   gcc -o server server.c -lsqlite3
   gcc -o client client.c

## Usage

1. **Run the server:**
   ```sh
   ./server

2. **Run the client:**
   ```sh
   ./client

3. **Client Authentication:**

- Enter the username, password, and resource when prompted.

- If the credentials are valid and the user has permission to access the resource, the client will receive the file descriptor and display the content of the shared file.

## Example Output
## Server Terminal:
   ```sh
   ./server

## Client Terminal:
   ```sh
   ./client
   Enter username: user1
   Enter password: password1
   Enter resource: shared_file.txt
   Server response: Authentication successful
   This is a shared file.

## Code Overview

## Server (server.c)
   ```sh
   - **verify_credentials:** Verifies user credentials against the SQLite database.
   - **check_permission:** Checks if the user has permission to access the requested resource.
   - **send_message:** Sends a message to the client.
   - **send_fd:** Sends a file descriptor to the client.
   - **handle_client:** Handles client requests, verifies credentials, checks permissions, and sends the file descriptor if authentication is successful.
   - **main:** Sets up the UNIX domain socket, uses select to manage concurrency, and handles client connections.

## Client (client.c)

   - **recv_fd:** Receives a file descriptor from the server.
   - **main:** Connects to the server, sends credentials and resource request, receives the server's response, and reads the content of the shared file if authentication is successful.

## License

This project is licensed under the MIT License. See the LICENSE file for details.

## Acknowledgements

SQLite: https://www.sqlite.org/
UNIX domain sockets: https://man7.org/linux/man-pages/man7/unix.7.html
File descriptor passing: https://man7.org/linux/man-pages/man3/cmsg.3.html

```sh

   Make sure to replace `https://github.com/yourusername/trusted_daemon.git` with the actual URL of your repository. This `README.md` file provides an overview of the project, setup instructions, usage details, and a brief code overview.
   Make sure to replace `https://github.com/yourusername/trusted_daemon.git` with the actual URL of your repository. This `README.md` file provides an overview of the project, setup instructions, usage details, and a brief code overview.


   **********************************************************************************************************

   /Users/indumathimadhu/trusted_daemon/AOS_Project/README.md