document.addEventListener('DOMContentLoaded', () => {
    // Initialize UI elements
    const wifiList = document.getElementById('Wifilist');
    const keySection = document.getElementById('keySection');
    const keyField = document.getElementById('keyfield');
    const connectButton = document.getElementById('connectButton');
    const connectButtonSpinner = connectButton.querySelector('.spinner-border');
    const connectButtonText = connectButton.querySelector('.button-text');
    const refreshButton = document.getElementById('refreshButton');
    const statusMessage = document.getElementById('statusMessage');

    // Utility function to show status messages
    const showStatus = (message, isError = false) => {
        statusMessage.textContent = message;
        statusMessage.className = isError ? 'status-error' : 'status-success';
        statusMessage.style.display = 'block';
    };

    // Utility function to hide elements
    const hide = (element) => { element.style.display = 'none'; };
    const show = (element) => { element.style.display = 'block'; };

    // Fetch WiFi list from the ESP32
    const updateWifiList = async () => {
        hide(keySection);
        wifiList.disabled = true;
        wifiList.innerHTML = '<option>Refreshing network list...</option>';

        try {
            const response = await fetch('/getWifiList');
            if (!response.ok) throw new Error('Failed to fetch networks.');
            
            const networks = await response.json();
            
            wifiList.innerHTML = '<option value="">Choose a network...</option>';
            networks.forEach(net => {
                const option = document.createElement('option');
                option.text = net.name;
                option.value = net.val;
                wifiList.add(option);
            });
        } catch (error) {
            showStatus('Could not load WiFi networks. Please refresh.', true);
            wifiList.innerHTML = '<option>Error loading networks</option>';
        } finally {
            wifiList.disabled = false;
        }
    };

    // Poll for connection status
    const pollConnectionStatus = async () => {
        try {
            const response = await fetch('/checkStatus');
            if (!response.ok) return 'WAIT'; // Assume busy if request fails
            
            const result = await response.text();
            console.log(result);
            return result;
        } catch (error) {
            console.log(error);
            return 'WAIT';
        }
    };

    // Attempt to connect to the new URL after reboot
    const attemptReconnect = (targetUrl) => {
        let attempts = 0;
        const maxAttempts = 60; // Try for ~2 minutes

        const check = async () => {
            try {
                const controller = new AbortController();
                const timeoutId = setTimeout(() => controller.abort(), 2000);
                await fetch(targetUrl, { mode: 'no-cors', cache: 'no-cache', signal: controller.signal });
                clearTimeout(timeoutId);
                window.location.href = targetUrl;
            } catch (error) {
                attempts++;
                if (attempts < maxAttempts) {
                    setTimeout(check, 2000);
                } else {
                    showStatus(`Device restarted. Please manually navigate to ${targetUrl}`, true);
                }
            }
        };
        
        // Initial delay to allow device to reboot and client to switch networks
        setTimeout(check, 5000);
    };

    // Handle the WiFi connection process
    const connectWifi = async () => {
        const ssid = wifiList.value;
        const password = keyField.value;

        if (!ssid) {
            showStatus('Please select a network.', true);
            return;
        }

        hide(statusMessage);
        connectButton.disabled = true;
        refreshButton.disabled = true;
        connectButtonSpinner.style.display = 'inline-block';
        connectButtonText.style.display = 'none';
        showStatus('Sending credentials to device...');

        const formData = new URLSearchParams();
        formData.append('ssidval', ssid);
        formData.append('keyval', password);

        try {
            const response = await fetch('/connectwWifi', { method: 'POST', body: formData });
            const responseText = await response.text();
            if (!response.ok || responseText.trim() !== 'RCVD') {
                throw new Error('Device did not accept credentials. Response: ' + responseText);
            }

            console.log(responseText);
            showStatus('Credentials accepted. Checking connection status...');
            
            // Poll for status
            let wasWaiting = false;
            const interval = setInterval(async () => {
                const status = await pollConnectionStatus();
                console.log('Connection status:', status);
                if (status === 'SUCCESS' || (status === 'IDLE' && wasWaiting)) {
                    clearInterval(interval);
                    showStatus('Success! Device is restarting. You must be connected to ' + ssid + ' in order to continue. Redirecting to http://sprinkler32.local ...');
                    attemptReconnect('http://sprinkler32.local');
                } else if (status === 'FAIL') {
                    clearInterval(interval);
                    showStatus('Connection failed. Please check the password and try again.', true);
                    connectButton.disabled = false;
                    refreshButton.disabled = false;
                    connectButtonSpinner.style.display = 'none';
                    connectButtonText.style.display = 'inline-block';
                }
                if (status === 'WAIT') {
                    wasWaiting = true;
                }
            }, 5000); // Poll every 5 seconds

        } catch (error) {
            console.log(error);
            showStatus(`Connection process failed: ${error.message}`, true);
            connectButton.disabled = false;
            refreshButton.disabled = false;
            connectButtonSpinner.style.display = 'none';
            connectButtonText.style.display = 'inline-block';
        }
    };

    // Event Listeners
    refreshButton.addEventListener('click', updateWifiList);
    connectButton.addEventListener('click', connectWifi);
    wifiList.addEventListener('change', () => {
        keyField.value = '';
        wifiList.value ? show(keySection) : hide(keySection);
    });
    document.getElementById('showPassword').addEventListener('change', (e) => {
        keyField.type = e.target.checked ? 'text' : 'password';
    });

    // Initial load
    updateWifiList();
});