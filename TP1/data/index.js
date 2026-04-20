const POLL_INTERVAL = 3000;

async function fetchState() {
    try {
        const res = await fetch('/state');
        if (!res.ok) return;
        const data = await res.json();

        document.getElementById('temp').textContent =
            data.temperature !== undefined ? data.temperature.toFixed(1) : '--';
        document.getElementById('humidity').textContent =
            data.humidity !== undefined ? data.humidity.toFixed(1) : '--';

        const ledOn = !!data.ledOn;
        const indicator = document.getElementById('ledIndicator');
        const label = document.getElementById('ledLabel');
        indicator.classList.toggle('on', ledOn);
        label.textContent = ledOn ? 'LED ON' : 'LED OFF';

        document.getElementById('lastUpdate').textContent =
            new Date().toLocaleTimeString();
    } catch (e) {
        console.error('Failed to fetch state:', e);
    }
}

async function toggleLed() {
    const btn = document.getElementById('ledBtn');
    btn.disabled = true;
    try {
        await fetch('/toggle-led', { method: 'POST' });
        await fetchState();
    } catch (e) {
        console.error('Failed to toggle LED:', e);
    } finally {
        btn.disabled = false;
    }
}

fetchState();
setInterval(fetchState, POLL_INTERVAL);