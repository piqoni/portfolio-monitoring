# portfolio-monitoring for ESP32 (T-Display S3)

![IMG_4487](https://github.com/user-attachments/assets/298c1385-a1db-4543-b0b3-a46cd013db52)

Personal project (use and/or modify as-is) to show in T-Display S3 device:
- Total Holdings
- Total Profit/Losses
- Daily Profit/Losses
- If daily losses go over 5%, it will flash in red
- Info refreshes every minute and stock data is retrieved from Yahoo Finance API. 

# Quickstart
 - Get the [hardware](https://lilygo.cc/products/t-display-s3?variant=42585826590901)
 - Setup platformio (I used [Vscode Extension](https://docs.platformio.org/en/latest/integration/ide/pioide.html#platformio-for-vscode))
 - Clone this repository and `cp dev.lots.yaml lots.yaml'. Edit your stock lots in that file
 - Open src/main.ino and on top are the **EDITME** variables that need to be edited (default currency, wifi network, etc)
 - Plug your device via usb and using Platformio Vscode Extension click Upload.
