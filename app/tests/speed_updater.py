#!/usr/bin/env python3
"""
Interactive Speed Updater for Quad Local Broker

This tool allows you to send speed updates to the Quad local broker
using the dndatamodel thing under vss.
"""

import requests
import json
import sys


def update_speed(speed_value: float, base_url: str = "http://localhost:50050") -> dict:
    """
    Send a speed update to the Quad local broker.
    
    Args:
        speed_value: The speed value to send (in km/h)
        base_url: The base URL of the Quad REST API
        
    Returns:
        Response from the server as a dictionary
    """
    url = f"{base_url}/reports/vss"
    headers = {
        "Content-Type": "application/json"
    }
    
    payload = {
        "item": {
            "dndatamodel": {
                "Vehicle": {
                    "EgoVehicle": {
                        "Motion": {
                            "Locomotion": {
                                "Speed": speed_value
                            }
                        }
                    }
                }
            }
        }
    }
    
    # Generate and display the equivalent curl command
    payload_str = json.dumps(payload)
    curl_cmd = f"curl -s -X PATCH {url} -H \"Content-Type: application/json\" -d '{payload_str}'"
    print(f"🔧 Executing: {curl_cmd}\n")
    
    try:
        response = requests.patch(url, headers=headers, json=payload, timeout=5)
        response.raise_for_status()
        return response.json()
    except requests.exceptions.RequestException as e:
        return {"error": str(e)}


def interactive_mode():
    """Run the tool in interactive mode."""
    print("=" * 60)
    print("Interactive Speed Updater for Quad Local Broker")
    print("=" * 60)
    print("\nThis tool sends speed updates to:")
    print("  URL: http://localhost:50050/reports/vss")
    print("  Path: dndatamodel/Vehicle/EgoVehicle/Motion/Locomotion/Speed")
    print("\nType 'quit' or 'exit' to exit, 'help' for more info\n")
    
    while True:
        try:
            user_input = input("Enter speed value (km/h): ").strip()
            
            if user_input.lower() in ['quit', 'exit', 'q']:
                print("Goodbye!")
                break
            
            if user_input.lower() == 'help':
                print("\nUsage:")
                print("  - Enter a numeric speed value (e.g., 120.5)")
                print("  - The value will be sent to the Quad broker")
                print("  - Type 'quit', 'exit', or 'q' to exit")
                print("  - Type 'help' to see this message\n")
                continue
            
            if not user_input:
                continue
            
            try:
                speed = float(user_input)
            except ValueError:
                print(f"❌ Error: '{user_input}' is not a valid number\n")
                continue
            
            print(f"📤 Sending speed: {speed} km/h...")
            result = update_speed(speed)
            
            if "error" in result:
                print(f"❌ Error: {result['error']}\n")
            elif result.get("code") == "OK":
                print(f"✅ Success! Speed updated to {speed} km/h")
                print(f"   Request ID: {result.get('requestId', 'N/A')}")
                print(f"   Message: {result.get('message', 'N/A')}\n")
            else:
                print(f"⚠️  Unexpected response: {result}\n")
                
        except KeyboardInterrupt:
            print("\n\nInterrupted. Goodbye!")
            break
        except Exception as e:
            print(f"❌ Unexpected error: {e}\n")


def main():
    """Main entry point."""
    if len(sys.argv) > 1:
        # Command-line mode: update speed from argument
        try:
            speed = float(sys.argv[1])
            print(f"Updating speed to {speed} km/h...")
            result = update_speed(speed)
            print(json.dumps(result, indent=2))
            sys.exit(0 if result.get("code") == "OK" else 1)
        except ValueError:
            print(f"Error: '{sys.argv[1]}' is not a valid number", file=sys.stderr)
            print("Usage: python speed_updater.py [speed_value]", file=sys.stderr)
            sys.exit(1)
    else:
        # Interactive mode
        interactive_mode()


if __name__ == "__main__":
    main()
