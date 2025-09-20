CREATE DATABASE cpp_chat;

USE cpp_chat;

CREATE TABLE users (
    name VARCHAR(50) PRIMARY KEY,
    password VARCHAR(50) NOT NULL
);

CREATE TABLE online_users (
    name VARCHAR(50) PRIMARY KEY,
    socket_fd INT NOT NULL,
    FOREIGN KEY (name) REFERENCES users(name)
);

CREATE TABLE offline_messages (
    id INT AUTO_INCREMENT PRIMARY KEY,
    to_user VARCHAR(50) NOT NULL,
    message TEXT NOT NULL,
    FOREIGN KEY (to_user) REFERENCES users(name)
);