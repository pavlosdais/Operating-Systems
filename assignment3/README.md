## Usage

- Compile project:
```bash
$ make
```

- Run the **readers-writers simulation**:
```bash
$ ./bin/creator -f <file> -rn <readers_number> -wn <writers_number> -rt <readers_time> -wt <writers_time> -wp <writer_exec> -rp <reader_exec> -sm <shared_memory_name> -lg <log_level>
```
**or**
```bash
$ make run
```

- Remove object files & executable program
```bash
$ make clear
```
