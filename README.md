# TourBox Native Node.js Addon

A native Node.js addon for receiving input from TourBox devices. This addon provides a JavaScript interface to interact with a TourBox when it is in Max/MSP mode.

## Features

* **Full TourBox Control Support** - All buttons, knobs, dials, and scroll wheels
* **High Performance** - Native C++ networking with JavaScript flexibility
* **Event-Driven API** - Simple event listeners for all controls
* **Button State Tracking** - Press/release events with held state monitoring
* **Combo Actions** - Create complex actions by combining multiple controls
* **Easy Integration** - Drop-in replacement for standalone C++ application

## Installation

### Prerequisites

- Node.js 16.0.0 or higher
- Windows (currently Windows-only, tested on Windows 10/11)
- TourBox Console software installed
- C++ build tools (automatically handled by node-gyp)

### Install Dependencies

```bash
cd nodejsmodule
npm install
```

This will automatically build the native addon using node-gyp.

### Manual Build (if needed)

```bash
npm run build
```

## Quick Start

```javascript
const tourbox = require('@brso/tourbox');

// Start the TourBox server on localhost (default)
tourbox.startServer(50500, "127.0.0.1");

// Or start on all interfaces to allow remote connections
// tourbox.startServer(50500, "0.0.0.0");

// Listen for knob rotation
tourbox.on('Knob CW', (count) => {
    console.log(`Knob turned clockwise ${count} times`);
});

tourbox.on('Knob CCW', (count) => {
    console.log(`Knob turned counter-clockwise ${count} times`);
});

// Listen for button presses
tourbox.on('C1 Press', (count) => {
    console.log('C1 button pressed');
});

tourbox.on('C1 Release', (count) => {
    console.log('C1 button released');
});

// OR use catch-all listener for any control
tourbox.on('*', (control, count) => {
    console.log(`Any control: ${control} (count: ${count})`);
});
```

## API Reference

### Methods

#### `tourbox.startServer(port, ip)`
Start the TourBox server.
- `port` (number, optional): Port to listen on (default: 50500)
- `ip` (string, optional): IP address to bind to (default: "127.0.0.1")
  - Use "127.0.0.1" for localhost only
  - Use "0.0.0.0" to accept connections from any IP address
- Returns: boolean - Success status

#### `tourbox.stopServer()`
Stop the TourBox server.
- Returns: boolean - Success status

#### `tourbox.isServerRunning()`
Check if the server is currently running.
- Returns: boolean - Running status

#### `tourbox.raw(callback)`
Set raw data callback to receive raw TourBox protocol data.
- `callback` (function): Function that receives Buffer objects with raw data
- Returns: TourBox instance (for chaining)

#### `tourbox.getAvailableControls()`
Get list of all available control names.
- Returns: string[] - Array of control names

#### `tourbox.buttonState(buttonname)`
Get the held/pressed state of the provided button name
- `buttonname`: A button string
- Returns: boolean - Held status

### Events

All events provide a `count` parameter indicating the number of actions (useful for rotation controls).

#### Catch-All Event Listener

Use the special `*` event to listen for ALL control events:

```javascript
tourbox.on('*', (control, count) => {
    console.log(`Control: ${control}, Count: ${count}`);
    
    // Handle all controls in one place
    switch(control) {
        case 'Knob CW':
            // Handle knob clockwise
            break;
        case 'C1 Press':
            // Handle C1 press
            break;
        default:
            // Handle any other control
    }
});
```

#### Knob Controls
- `Knob CW` - Knob turned clockwise
- `Knob CCW` - Knob turned counter-clockwise  
- `Knob Press` - Knob pressed down
- `Knob Release` - Knob released

#### Scroll Wheel
- `Scroll Up` - Scroll wheel turned up
- `Scroll Down` - Scroll wheel turned down
- `Scroll Press` - Scroll wheel pressed
- `Scroll Release` - Scroll wheel released

#### Dial Controls
- `Dial CW` - Dial turned clockwise
- `Dial CCW` - Dial turned counter-clockwise
- `Dial Press` - Dial pressed down
- `Dial Release` - Dial released

#### Directional Pad
- `Up Press` / `Up Release`
- `Down Press` / `Down Release`
- `Left Press` / `Left Release`
- `Right Press` / `Right Release`

#### Side Buttons
- `Tall Press` / `Tall/Side Release`
- `Side Press` / `Tall/Side Release` (shared release)
- `Short Press` / `Short Release`
- `Top Press` / `Top Release`

#### Function Buttons
- `Tour Press` / `Tour Release`
- `C1 Press` / `C1 Release`
- `C2 Press` / `C2 Release`

#### Connection Events
- `connect` - TourBox device connected (provides connection info object)
- `disconnect` - TourBox device disconnected (provides connection info object)

## Advanced Usage

### Combo Actions

```javascript
let isC1Held = false;
let isC2Held = false;

// Create combo action
tourbox.on('Knob CW', (count) => {
    if (tourbox.buttonState("C1") && tourbox.buttonState("C2")) {
        console.log('Special combo: C1+C2+Knob');
        // Your custom action here
    } else if (tourbox.buttonState("C1")) {
        console.log('C1 + Knob action');
    } else {
        console.log('Normal knob action');
    }
});
```

### FL Studio Integration Example

```javascript
const { exec } = require('child_process');

// Example: Control FL Studio playback
tourbox.on('Tour Press', () => {
    // Send spacebar to FL Studio (play/pause)
    exec('nircmd sendkey "FL Studio" 0x20'); // Spacebar
});

tourbox.on('Knob CW', (count) => {
    // Move selection right
    for (let i = 0; i < count; i++) {
        exec('nircmd sendkey "FL Studio" 0x27'); // Right arrow
    }
});
```

### Custom MIDI/OSC Integration

```javascript
const midi = require('midi');
const output = new midi.Output();

// Connect to MIDI device
output.openPort(0);

tourbox.on('Knob CW', (count) => {
    // Send MIDI CC message
    output.sendMessage([176, 1, Math.min(127, count * 10)]);
});
```

## Testing

Run the test file to verify everything works:

```bash
npm test
```

This will start the server and log all TourBox events to the console.

## Troubleshooting

### Build Issues

If you encounter build errors:

1. Ensure you have Windows Build Tools installed:
   ```bash
   npm install -g windows-build-tools
   ```

2. Try rebuilding:
   ```bash
   npm run clean
   npm run build
   ```

### Connection Issues

1. Make sure TourBox Console is running
2. Check that no other applications are using port 50500
3. Verify your TourBox device is connected and an Max/MSP preset is active in TourBox Console

### Permission Issues

Run Node.js as Administrator if you encounter permission errors.

## Architecture

The addon consists of:

- **C++ Server** (`tourbox_server.cc`) - Handles TCP socket connections
- **C++ Client** (`tourbox_client.cc`) - Processes TourBox protocol data  
- **Node.js Interface** (`index.js`) - JavaScript event emitter wrapper
- **Native Bindings** (`tourbox_addon.cc`) - Node-API integration

## License

MIT License - Feel free to use in your projects!

## Contributing

Pull requests welcome! Please ensure all tests pass and follow existing code style.
