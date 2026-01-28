import socket

# Server address and port
SERVER_HOST = '37.46.135.97'
SERVER_PORT = 3210

try:
    # Create a socket object
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # Connect to the server
    client_socket.connect((SERVER_HOST, SERVER_PORT))
    print(f"Connected to {SERVER_HOST}:{SERVER_PORT}")

    # Send data to the server
    message = "Hello from the client!"
    client_socket.sendall(message.encode())

    # Receive data from the server
    data = client_socket.recv(8192)
    print(f"Received from server: {data.decode()}")

except ConnectionRefusedError:
    print(f"Connection refused. Make sure the server is running on {SERVER_HOST}:{SERVER_PORT}")
except Exception as e:
    print(f"An error occurred: {e}")
finally:
    # Close the socket
    if 'client_socket' in locals() and client_socket:
        client_socket.close()
        print("Connection closed.")