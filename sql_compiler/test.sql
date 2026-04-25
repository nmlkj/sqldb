CREATE TABLE users (id INT PRIMARY KEY, name TEXT);
INSERT INTO users (id, name) VALUES (1, 'Alice');
SELECT * FROM users;
SELECT id, name FROM users WHERE id = 1;
DROP TABLE users;
