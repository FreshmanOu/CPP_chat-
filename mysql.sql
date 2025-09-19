-- 创建SECMNG模式 
CREATE SCHEMA IF NOT EXISTS SECMNG; 

USE SECMNG; 

-- 创建密钥序列号表 
CREATE TABLE IF NOT EXISTS KEYSN ( 
    ikeysn INT PRIMARY KEY 
); 

-- 插入初始密钥序列号 
INSERT INTO KEYSN (ikeysn) VALUES (1); 

-- 创建网点信息表 --编号 名称 描述 授权码 状态(0可用 1不可用) 
CREATE TABLE IF NOT EXISTS SECNODE( 
    id          CHAR(4) PRIMARY KEY, 
    name        VARCHAR(128) NOT NULL, 
    nodedesc    VARCHAR(512), 
    createtime  DATETIME, 
    authcode    INT, 
    state       INT 
); 

-- 插入网点信息数据 
INSERT INTO SECMNG.SECNODE VALUES('2256', '客户端2256', '测试节点', '2025-07-1 00:00:00', 0, 0); 
INSERT INTO SECMNG.SECNODE VALUES('1111', '广东分中心1111', '测试节点', '2025-07-1 00:00:00', 0, 0); 

-- 创建密钥信息表，客户端网点 服务器端网点 密钥号 密钥产生时间 密钥状态 
CREATE TABLE IF NOT EXISTS SECKEYINFO( 
    clientid    CHAR(4), 
    serverid    CHAR(4), 
    keyid       INT PRIMARY KEY AUTO_INCREMENT, 
    createtime  DATETIME, 
    state       INT, 
    seckey      VARCHAR(512), 
    FOREIGN KEY (clientid) REFERENCES SECNODE(id), 
    FOREIGN KEY (serverid) REFERENCES SECNODE(id) 
); 

-- 创建索引 
CREATE INDEX IF NOT EXISTS IX_SECKEYINFO_clientid ON SECKEYINFO(clientid); 

-- 创建触发器，在插入新密钥时自动更新密钥序列号 
DELIMITER // 
CREATE TRIGGER update_keysn_after_insert 
AFTER INSERT ON SECKEYINFO 
FOR EACH ROW 
BEGIN 
    UPDATE KEYSN SET ikeysn = NEW.keyid + 1; 
END// 
DELIMITER ; 

-- 创建存储过程，获取下一个密钥ID 
DELIMITER // 
CREATE PROCEDURE GetNextKeyID(OUT keyID INT) 
BEGIN 
    -- 锁定表 
    LOCK TABLES KEYSN WRITE; 
    
    -- 获取当前密钥ID 
    SELECT ikeysn INTO keyID FROM KEYSN; 
    
    -- 更新密钥ID 
    UPDATE KEYSN SET ikeysn = ikeysn + 1; 
    
    -- 解锁表 
    UNLOCK TABLES; 
END// 
DELIMITER ; 

-- 创建视图，便于查询有效的密钥 
CREATE VIEW IF NOT EXISTS ACTIVE_SECKEYS AS 
SELECT s.clientid, s.serverid, s.keyid, s.createtime, s.seckey, 
       c1.name as client_name, c2.name as server_name 
FROM SECKEYINFO s 
JOIN SECNODE c1 ON s.clientid = c1.id 
JOIN SECNODE c2 ON s.serverid = c2.id 
WHERE s.state = 0; -- 0表示可用，与Oracle一致 

-- 创建用户并授权 
-- CREATE USER 'sec_user'@'%' IDENTIFIED BY 'sec_password'; 
-- GRANT SELECT, INSERT, UPDATE, DELETE, EXECUTE ON SECMNG.* TO 'sec_user'@'%'; 
-- FLUSH PRIVILEGES; 

-- 提交事务 
COMMIT;