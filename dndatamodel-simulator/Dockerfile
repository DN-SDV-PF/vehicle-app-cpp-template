FROM python:3.9-slim

WORKDIR /app

# Install required Python packages
RUN pip install --no-cache-dir requests flask flask-cors

# Copy simulator with web UI and templates
COPY simulator_web.py /app/simulator_web.py
COPY templates/ /app/templates/
RUN chmod +x /app/simulator_web.py

# Expose port 8081 for web UI
EXPOSE 8081

# Run simulator with web UI and autostart
CMD ["python", "simulator_web.py", "--quad-url", "http://localhost:50050", "--port", "8081", "--interval", "2.0", "--autostart"]
