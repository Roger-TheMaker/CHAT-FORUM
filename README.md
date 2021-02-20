CHAT ROOM using BSD Sockets(UNIX)

Components:   
  -> Concurrent Server    
  -> Clients 

Server and Client Functionalities:   
  
Server:  Manage a chat room by accepting multiple simultaneous connections from users who want to chat.
         Users must log in with a name and password. The name must be unique.
         The server will receive messages from connected users and will immediately send the received messages to all clients.
         
Client:  It connects to the server and offers a text interface through which he can send messages to the chat room.
         A message sent by a user will appear simultaneously in the interface provided to all users, including the one who originally wrote the message.
          
   
   
