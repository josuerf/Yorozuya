# BossLog

Addon for RFOnline that logs and notifies the spawn and death of bosses on the server.

## Description

BossLog is a module that monitors the creation and destruction of boss monsters on the server, sending broadcast messages to all online players when a boss spawns or is defeated.

## Features

- **Spawn Log**: Notifies when a boss appears on the map  
- **Death Log**: Notifies when a boss is defeated  
- **Killer Information**: Optionally displays the name of the player who defeated the boss  
- **Custom Names**: Supports a JSON file with custom names for monsters  
- **Configurable**: Allows enabling or disabling each feature individually  

## Configuration

The addon is configured through a JSON configuration file. Example:

```json
{
  "name": "addon.boss_log",
  "config": {
    "activated": true,
    "log_birth": true,
    "log_death": true,
    "log_killer_name": true,
    "monsters_json_file_path": "./YorozuyaGS/Data/BossLog/monsters.json"
  }
}
```

### Configuration Parameters

- **`activated`** (boolean): Enables or disables the addon completely  
  - Default: `false`
  
- **`log_birth`** (boolean): Logs and notifies when a boss spawns  
  - Default: `true`
  
- **`log_death`** (boolean): Logs and notifies when a boss is defeated  
  - Default: `true`
  
- **`log_killer_name`** (boolean): Displays the name of the player who defeated the boss  
  - Default: `true`
  
- **`monsters_json_file_path`** (string): Path to the JSON file with custom monster names  
  - Default: `""` (empty)  
  - If not specified or empty, bosses will be identified as "Unknown"  

### Monster Names File

The JSON file of names must be an array of strings, where each index corresponds to the monster index in the database:

```json
[
  "Brath",
  "Belphegor",
  "Calliana Princess",
]
```

**Important**: The index in the array must correspond to the monster’s `m_dwIndex` field in the record (`MonRec`).  
If the index does not exist in the array or is empty, the system will use "Unknown" as the default name.

## Messages

### Boss Spawn
When a boss spawns, the message sent will be:
```
Boss [Boss Name] has spawned in [Map Name]!
```

### Boss Death
When a boss is defeated, the message sent will be:
```
Boss [Boss Name] was defeated by [Player Name] in [Map Name]!
```

If `log_killer_name` is disabled, the message will be:
```
Boss [Boss Name] was defeated in [Map Name]!
```

## Requirements

- RFOnline Server version 2.2.3.2  
- Yorozuya Framework  
- Visual Studio 2017 15.5.2 or higher  
- Visual Studio x64 toolset  

## Compilation

The addon is compiled as part of the main Yorozuya project. Make sure all dependencies are installed and the project is properly configured.

## Usage

1. Configure the addon in the server configuration file  
2. (Optional) Create a JSON file with the boss names  
3. Compile and place the `BossLog.dll` file in the server’s addons folder  
4. Restart the server to load the addon  

## Notes

- Only monsters marked as boss (`IsBossMonster()`) are monitored  
- Messages are sent only to players who are online at the time of the event  
- The addon checks if the attacker is a player before registering the death  
- Messages are broadcast using the server’s GM message system  