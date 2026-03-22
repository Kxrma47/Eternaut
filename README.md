# Linked List Serialization

Standalone C++ solution for the task:

- read `inlet.in`
- build a doubly linked list of `ListNode`
- restore `rand` pointers from indexes
- serialize the list into binary `outlet.out`

Deserialization is also implemented and can be checked with an optional verification run.

## Build

```bash
make
```

## Run

Place `inlet.in` in the directory where you run the program.

Required flow:

```bash
./inter_list
```

Optional round-trip verification:

```bash
./inter_list --verify
```

## Input format

Each line of `inlet.in`:

```text
<data>;<rand_index>
```

The parser splits by the last `;`, so `data` may itself contain semicolons.

## Binary format

`outlet.out` stores:

1. number of nodes
2. for each node:
   - string length
   - raw string bytes
   - `rand_index`

Raw pointer values are never written to the file.
