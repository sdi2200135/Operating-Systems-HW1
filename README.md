# Operating Systems-HW1

---

## 1. Introduction

This project implements an **inter-process communication system** on Linux,
built on top of **shared memory** and **System V semaphores**. Each process
can create a new dialogue or join an existing one, send and receive messages
from other processes, and terminate the dialogue by sending a `TERMINATE`
message.

The code is split into several files to handle shared memory, the message
queue, dialogue control, and synchronization. Files are organised into two
folders: header files (`.h`) in `include/` and implementation files (`.c`) in
`src/`. A `Makefile` and this `README` are also provided.

---

## 2. File Structure

### 2.1 Data Structures

All core data structures are declared in `common.h`:

1. **`MSG`** – Represents a single message. It contains:
   - a unique message id,
   - the message content,
   - the sender's pid,
   - an array of readers (pids),
   - the number of readers so far,
   - the number of participants at the time the message was sent,
   - a `terminated` flag (set when the message is `TERMINATE`).

2. **`MSG_queue`** – The message queue of a dialogue. It contains:
   - an array of `MSG` items,
   - a `head` pointer to the most recent message,
   - a `tail` pointer to the oldest message,
   - the current number of messages in the queue,
   - the next available message id.

3. **`Dialog`** – Represents a dialogue. It contains:
   - the dialogue id,
   - the current number of participants,
   - an array of participant pids,
   - the message queue,
   - the id of the dialogue's semaphore set,
   - an `actived` flag indicating whether the dialogue is active.

4. **`Dialog_list`** – Tracks all active dialogues. It contains:
   - an array of dialogue ids,
   - the number of active dialogues,
   - the id of the semaphore protecting the list.

### 2.2 Files

| File | Purpose |
|------|---------|
| `include/common.h` | Data structures, constants, IPC keys, macros |
| `include/shared_memory.h` | Shared-memory management |
| `include/synchronization.h` | Semaphore-set management |
| `include/dialog_control.h` | Dialogue management |
| `include/msg_queue.h` | Message queue management |
| `src/shared_memory.c` | Implementation of shared-memory functions |
| `src/synchronization.c` | Implementation of semaphore functions |
| `src/dialog_control.c` | Implementation of dialogue functions |
| `src/msg_queue.c` | Implementation of message-queue functions |
| `src/main.c` | Main program + user interaction |

---

## 3. Synchronization Tools

### 3.1 Per-dialogue semaphores

For each dialogue a **set of 3 semaphores** is created:

- **`sem_access_dialog`** – for exclusive access to the `Dialog` structure.
- **`sem_empty_slots`** – counts the free slots in the message queue.
- **`sem_full_slots`** – counts the full slots in the message queue.

### 3.2 Macros

For convenience, `common.h` defines macros that lock/unlock the dialogue,
wait/signal for empty slots, and wait/signal for full slots. All semaphore
operations use **`SEM_UNDO`** so that if a process terminates unexpectedly,
the semaphores are automatically restored.

---

## 4. System Operations

### 4.1 Startup and joining a dialogue

Each process attaches to the dialogue list and, from a menu, can:

1. **Create** a new dialogue with a unique random id (added to the list).
2. **Join** an existing dialogue by entering its id.
3. **List** all active dialogues.
4. **Exit** the system.

### 4.2 Sending a message

The system waits for an empty slot, then locks the dialogue. It copies the
message into the queue, sets the sender, message id, and the `terminated`
flag if needed. The sender is marked as the first reader of the message, the
queue pointers are updated, and a signal is sent for the new message.

### 4.3 Receiving a message

The system locks the dialogue and walks the message queue looking for
messages not yet read by the current process. If it finds one, it copies the
message, adds the process's pid to the reader array, and if the message has
been read by everyone, advances the `tail` pointer and signals an empty slot.

### 4.4 Terminating a dialogue

There are two ways to terminate a dialogue:

1. **Sending `TERMINATE`** – the flag is set to 1, and once all participants
   have received the message, `clean_all()` is called to detach and clean up.
2. **Choosing `E` from the menu** – `leave_dialog()` removes the process from
   the dialogue; when no participants remain, the dialogue is removed from
   the list and destroyed.

---

## 5. Memory Management and IPC

### 5.1 Unique key generation

`generate_key()` produces a unique key by adding:

- `IPC_PRIVATE`,
- the dialogue id,
- and a base value, which is one of:
  - `key_list`   – for the dialogue list,
  - `key_sem`    – for semaphores,
  - `key_dialog` – for the shared memory of a dialogue.

### 5.2 System cleanup

- `detach_d_list()` / `detach_dialog()` – detach from shared memory.
- `destroy_d_list()` / `destroy_dialog()` – remove shared-memory segments.
- `clean_all()` – calls both detach functions.

---

## 6. Design Choices

### 6.1 Readers per message

Each `MSG` contains a `readers` array and a `readers_c` counter. When a
process reads a message, its pid is added to the array. The function
`msg_read_by_all()` compares the reader array with the current participants
of the dialogue to ensure each message is read exactly once by each
participant.

### 6.2 Message queue

Pointers (`head`, `tail`, `count`) are used to make it easy to shift messages
in the queue and to check its size.

### 6.3 Handling `TERMINATE`

The last participant of a dialogue is responsible for calling the functions
that delete the dialogue and the dialogue list. This guarantees that
termination does not occur before every participant has read all messages.

---

## 7. Build & Run

| Command | Purpose |
|---------|---------|
| `make all` | Compile all necessary files |
| `make run` | Run the program (open additional terminals and run `make run` in each) |
| `make clean` | Delete the generated files |

To enable communication between processes, run `make run` in **multiple
terminals**.
