# TODO

[x]. Написать простую структуру данных для поля судоку в виде хедера (используется и на клиенте, и на сервере)

- [ ]. Написать клиент
  - [ ]. Написать сервер так, чтобы он взаимодействовал с клиентом (возвращал хотя бы dummy поле)
    [ ]. ..



- так как сервер должен дождаться обслуживания всех клиентов, в момент получения SIGTERM 

## Общие моменты для клиента и сервера

Общие функции и структуры для клиента и сервера должны подключаться через хедеры

### Поле для судоку

```c


typedef struct sudoku_field {
    
```



## Сервер

## Клиент

1. Отправка серверу сообщения `generate`
	- сообщение только одно, можно захардкодить
	- нужна только структура 
