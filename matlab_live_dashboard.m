clc;
clear;
close all;

%% COGNITIVE RADIO LIVE DASHBOARD
% Receives DATA_CSV telemetry from the ESP8266 receiver.
% DHT11 valid operating range: approximately 0 to 50 degrees C.

port = "COM5";
baudRate = 115200;

% Close the Arduino Serial Monitor before running this script.
s = serialport(port, baudRate);
configureTerminator(s, "LF");
flush(s);

% Live data storage
t = [];
packetID = [];
activeChannel = [];
temperature = [];
received = [];
lost = [];

ap1 = []; rssi1 = []; score1 = [];
ap6 = []; rssi6 = []; score6 = [];
ap11 = []; rssi11 = []; score11 = [];

startTime = tic;

%% Dashboard layout
fig = figure( ...
    'Name', 'Cognitive Radio Live Dashboard', ...
    'NumberTitle', 'off', ...
    'Color', 'w');

tl = tiledlayout(fig, 3, 2, ...
    'TileSpacing', 'compact', ...
    'Padding', 'compact');

ax1 = nexttile(tl);
h1 = plot(ax1, NaN, NaN, 'LineWidth', 1.5);
title(ax1, 'DHT11 Temperature vs Time');
xlabel(ax1, 'Time (s)');
ylabel(ax1, 'Temperature (°C)');
grid(ax1, 'on');

ax2 = nexttile(tl);
h2 = plot(ax2, NaN, NaN, 'LineWidth', 1.5);
title(ax2, 'Received Packets vs Time');
xlabel(ax2, 'Time (s)');
ylabel(ax2, 'Packet Count');
grid(ax2, 'on');

ax3 = nexttile(tl);
h3 = plot(ax3, NaN, NaN, 'LineWidth', 1.5);
hold(ax3, 'on');
h4 = plot(ax3, NaN, NaN, 'LineWidth', 1.5);
h5 = plot(ax3, NaN, NaN, 'LineWidth', 1.5);
title(ax3, 'Average Wi-Fi RSSI by Channel');
xlabel(ax3, 'Time (s)');
ylabel(ax3, 'Average RSSI (dBm)');
legend(ax3, 'Channel 1', 'Channel 6', 'Channel 11', ...
    'Location', 'best');
grid(ax3, 'on');

ax4 = nexttile(tl);
h6 = plot(ax4, NaN, NaN, 'LineWidth', 1.5);
hold(ax4, 'on');
h7 = plot(ax4, NaN, NaN, 'LineWidth', 1.5);
h8 = plot(ax4, NaN, NaN, 'LineWidth', 1.5);
title(ax4, 'Detected Wi-Fi Networks');
xlabel(ax4, 'Time (s)');
ylabel(ax4, 'Access Point Count');
legend(ax4, 'Channel 1', 'Channel 6', 'Channel 11', ...
    'Location', 'best');
grid(ax4, 'on');

ax5 = nexttile(tl);
h9 = plot(ax5, NaN, NaN, 'LineWidth', 1.5);
hold(ax5, 'on');
h10 = plot(ax5, NaN, NaN, 'LineWidth', 1.5);
h11 = plot(ax5, NaN, NaN, 'LineWidth', 1.5);
title(ax5, 'Channel Selection Scores');
xlabel(ax5, 'Time (s)');
ylabel(ax5, 'Score');
legend(ax5, 'Channel 1', 'Channel 6', 'Channel 11', ...
    'Location', 'best');
grid(ax5, 'on');

ax6 = nexttile(tl);

yyaxis(ax6, 'left');
h12 = stairs(ax6, NaN, NaN, 'LineWidth', 1.5);
ylabel(ax6, 'Active Channel');
ylim(ax6, [0 12]);
yticks(ax6, [1 6 11]);

yyaxis(ax6, 'right');
h13 = plot(ax6, NaN, NaN, '--', 'LineWidth', 1.5);
ylabel(ax6, 'Lost Packet Count');

title(ax6, 'Channel Switching and Packet Loss');
xlabel(ax6, 'Time (s)');
grid(ax6, 'on');

sgtitle(tl, 'Cognitive Radio - Live Hardware Monitoring');

disp('Dashboard running. Close the figure or press Ctrl+C to stop.');

%% Read receiver DATA_CSV continuously
while isvalid(fig)

    line = strtrim(readline(s));

    % Ignore ordinary debug lines.
    if ~startsWith(line, "DATA_CSV,")
        continue;
    end

    % Split the CSV record into individual numeric fields.
    fields = split(extractAfter(line, "DATA_CSV,"), ",");
    values = str2double(fields).';

    % Expected: exactly 14 numeric values.
    if numel(values) ~= 14 || any(~isfinite(values))
        disp("Skipping invalid CSV record.");
        continue;
    end

    nowTime = toc(startTime);

    % Decode the established CSV field order.
    id = values(1);
    ch = values(2);
    rawTemperature = values(3);
    rxCount = values(4);
    lostCount = values(5);

    a1 = values(6);
    r1 = values(7);
    sc1 = values(8);

    a6 = values(9);
    r6 = values(10);
    sc6 = values(11);

    a11 = values(12);
    r11 = values(13);
    sc11 = values(14);

    % DHT11 operating range is approximately 0 to 50 C.
    % Invalid temperature is excluded from the plot, but the
    % rest of the received packet is still processed.
    if rawTemperature >= 0 && rawTemperature <= 50
        validTemperature = rawTemperature;
    else
        validTemperature = NaN;
    end

    % Store measurements.
    t(end+1,1) = nowTime;
    packetID(end+1,1) = id;
    activeChannel(end+1,1) = ch;
    temperature(end+1,1) = validTemperature;
    received(end+1,1) = rxCount;
    lost(end+1,1) = lostCount;

    ap1(end+1,1) = a1;
    rssi1(end+1,1) = r1;
    score1(end+1,1) = sc1;

    ap6(end+1,1) = a6;
    rssi6(end+1,1) = r6;
    score6(end+1,1) = sc6;

    ap11(end+1,1) = a11;
    rssi11(end+1,1) = r11;
    score11(end+1,1) = sc11;

    % Update the six graphs.
    set(h1, 'XData', t, 'YData', temperature);
    set(h2, 'XData', t, 'YData', received);

    set(h3, 'XData', t, 'YData', rssi1);
    set(h4, 'XData', t, 'YData', rssi6);
    set(h5, 'XData', t, 'YData', rssi11);

    set(h6, 'XData', t, 'YData', ap1);
    set(h7, 'XData', t, 'YData', ap6);
    set(h8, 'XData', t, 'YData', ap11);

    set(h9, 'XData', t, 'YData', score1);
    set(h10, 'XData', t, 'YData', score6);
    set(h11, 'XData', t, 'YData', score11);

    set(h12, 'XData', t, 'YData', activeChannel);
    set(h13, 'XData', t, 'YData', lost);

    drawnow limitrate;

    % Display a clear status message in MATLAB.
    if isnan(validTemperature)
        fprintf(['Channel: %d | Temp: INVALID (raw %.2f C) | ' ...
                 'RX: %.0f | Lost: %.0f\n'], ...
                 ch, rawTemperature, rxCount, lostCount);
    else
        fprintf(['Channel: %d | Temp: %.1f C | ' ...
                 'RX: %.0f | Lost: %.0f\n'], ...
                 ch, validTemperature, rxCount, lostCount);
    end
end

clear s;
disp('Dashboard stopped.');
