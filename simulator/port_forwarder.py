import socket
import threading

def handle_client(client_socket, destination_host, destination_port):
    """Handles an incoming client connection by forwarding it to the destination."""
    try:
        # Connect to the destination server
        remote_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        remote_socket.connect((destination_host, destination_port))

        # Start two threads to continuously forward data in both directions
        thread_to_remote = threading.Thread(target=forward_data, args=(client_socket, remote_socket))
        thread_to_client = threading.Thread(target=forward_data, args=(remote_socket, client_socket))

        thread_to_remote.start()
        thread_to_client.start()

        # Join the threads to wait for them to finish
        thread_to_remote.join()
        thread_to_client.join()

    except Exception as e:
        print(f"Error handling client: {e}")
    finally:
        client_socket.close()
        remote_socket.close()

def forward_data(source_socket, destination_socket):
    """Continuously reads data from source and sends it to destination."""
    try:
        while True:
            data = source_socket.recv(4096)  # Read up to 4096 bytes
            if not data:
                break  # No more data, connection closed
            destination_socket.sendall(data)
    except Exception as e:
        print(f"Error forwarding data: {e}")

def start_port_forwarder(listen_host, listen_port, destination_host, destination_port):
    """Starts the port forwarder server."""
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((listen_host, listen_port))
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.listen(5)  # Listen for up to 5 queued connections

    print(f"Listening on {listen_host}:{listen_port} and forwarding to {destination_host}:{destination_port}")

    while True:
        client_socket, addr = server_socket.accept()
        print(f"Accepted connection from {addr[0]}:{addr[1]}")
        client_handler = threading.Thread(target=handle_client,
                                           args=(client_socket, destination_host, destination_port))
        client_handler.start()

if __name__ == "__main__":
    # Example usage: Forward connections from localhost:8080 to localhost:80
    LISTEN_HOST = '127.0.0.1'
    LISTEN_PORT = 40002
    DESTINATION_HOST = '127.0.0.1'
    DESTINATION_PORT = 40001

    start_port_forwarder(LISTEN_HOST, LISTEN_PORT, DESTINATION_HOST, DESTINATION_PORT)