# MLX90640


## 1. Requirement
- **driver** 

  CH340

- **OpenCV**

  

## 2. Serial Port

```
PACKET:
+-------+-------+--------------+-------------+-------------+
|HEADER | SIZE  | TEMPERATUES  |    TA       |  CHECKSUM   |
+---+---+---+---+---+---+------+------+------+------+------+
| 0 | 1 | 2 | 3 | 4 |...| 1539 | 1540 | 1541 | 1542 | 1543 |
+---+---+---+---+---+---+------+------+------+------+------+
HEADER:         0x5a,0x5a

SIZE:           size = byte3*256 + byte2

TEMPERATUES:    T1 = (byte5*256 + byte4)/100,
            	T768 = (byte1539*256 + byte1538)/100

TA:             TA = (byte1541*256+ byte1540)/100
```



## 3. Emissivity

| materials | emissivity  |
| --------- | ----------- |
| iron      | 0.6 - 0.9   |
| wood      | 0.9 - 0.95  |
| water     | 0.93        |
| plastic   | 0.85 - 0.95 |
| concrete  | 0.95        |
| dirt      | 0.9 - 0.98  |
| paper     | 0.94        |
| skin      | 0.98        |

