import socket
import threading
from tkinter import Tk, Label, Button, Listbox, END

# Server configuration
SERVER_IP = "127.0.0.1"
SERVER_PORT = 8080

# Connected clients
clients = []

# Start the server
def start_server():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((SERVER_IP, SERVER_PORT))
    server_socket.listen(5)
    print("Server started. Waiting for clients...")

    while True:
        client_socket, client_address = server_socket.accept()
        print(f"Connected to {client_address}")
        clients.append(client_address)
        update_client_list()

        # Handle client messages
        data = client_socket.recv(1024).decode()
        if data == "HEARTBEAT":
            print(f"Heartbeat received from {client_address}")

# Update the client list in the GUI
def update_client_list():
    client_list.delete(0, END)
    for client in clients:
        client_list.insert(END, f"{client[0]}:{client[1]}")

# Request a screenshot from a client
def request_screenshot():
    selected_index = client_list.curselection()
    if selected_index:
        selected_client = clients[selected_index[0]]
        print(f"Requesting screenshot from {selected_client}")

# Create the GUI
root = Tk()
root.title("Employee Monitoring Server")

client_list = Listbox(root, width=50, height=10)
client_list.pack()

refresh_button = Button(root, text="Refresh", command=update_client_list)
refresh_button.pack()

screenshot_button = Button(root, text="Request Screenshot", command=request_screenshot)
screenshot_button.pack()

start_server_thread = threading.Thread(target=start_server)
start_server_thread.daemon = True
start_server_thread.start()

root.mainloop()