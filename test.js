const tourbox = require('./index.js');

console.log('TourBox Node.js Test');
console.log('=======================\n');

// List available controls
console.log('Available controls:');
tourbox.getAvailableControls().forEach(control => {
    console.log(`   - ${control}`);
});
console.log('');

// Raw data listener - receives raw protocol data as Buffer
tourbox.raw((buffer) => {
    const hex = buffer.toString('hex');
    const bytes = Array.from(buffer).join(' ');
    console.log(`RAW DATA: ${hex} (bytes: ${bytes}) [${buffer.length} bytes]`);
});

// Connection events
tourbox.on('connect', (info) => {
    console.log(`TourBox CONNECTED from ${info.ip}:${info.port}`);
});

tourbox.on('disconnect', (info) => {
    console.log(`TourBox DISCONNECTED from ${info.ip}:${info.port}`);
});

// Catch-all event listener - receives ALL events
tourbox.on('*', (control, data) => {
    if (control === 'connect' || control === 'disconnect') {
    console.log(`CONNECTION: ${control} from ${data.ip}:${data.port}`);
    } else {
    console.log(`PARSED: ${control} (count: ${data})`);
    }
});

console.log('Raw data listener active - will show protocol data!');
console.log('Catch-all listener active - will log ALL parsed events!\n');

// Set up specific event listeners (optional - you can use just catch-all)
tourbox.on('Knob CCW', (count) => {
    console.log(`C1 button state is ${tourbox.buttonState('C1') ? 'held' : 'released'}`);
    console.log(`Knob turned counter-clockwise ${count} times`);
});

tourbox.on('Knob CW', (count) => {
    console.log(`Knob turned clockwise ${count} times`);
});

tourbox.on('Knob Press', (count) => {
    console.log(`Knob pressed`);
});

tourbox.on('Knob Release', (count) => {
    console.log(`Knob released`);
});

tourbox.on('Scroll Up', (count) => {
    console.log(`Scrolled up ${count} times`);
});

tourbox.on('Scroll Down', (count) => {
    console.log(`Scrolled down ${count} times`);
});

// Handle button presses
tourbox.on('C1 Press', (count) => {
    console.log(`C1 button pressed`);
});

tourbox.on('C1 Release', (count) => {
    console.log(`C1 button released`);
});

tourbox.on('C2 Press', (count) => {
    console.log(`C2 button pressed`);
});

tourbox.on('C2 Release', (count) => {
    console.log(`C2 button released`);
});

// Directional pad
tourbox.on('Up Press', (count) => {
    console.log(`Up pressed`);
});

tourbox.on('Down Press', (count) => {
    console.log(`Down pressed`);
});

tourbox.on('Left Press', (count) => {
    console.log(`Left pressed`);
});

tourbox.on('Right Press', (count) => {
    console.log(`Right pressed`);
});


// Start the server
console.log('Starting TourBox server...');
// Start server on localhost (default)
// tourbox.startServer(50500, "127.0.0.1");

// Or start server on all interfaces (allows remote connections)
// tourbox.startServer(50500, "0.0.0.0");

if (tourbox.startServer()) {
    console.log('Server started successfully!');
    console.log('Connect your TourBox and try using the controls...');
    console.log('Press Ctrl+C to stop\n');
} else {
    console.error('Failed to start server');
    process.exit(1);
}

// Graceful shutdown
process.on('SIGINT', () => {
    console.log('\nShutting down...');
    if (tourbox.stopServer()) {
    console.log('Server stopped successfully');
    }
    process.exit(0);
});