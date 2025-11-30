#!/usr/bin/env python3
"""
VSS Path Adapter Service
Translates between COVESA VSS paths and custom data model paths.
Subscribes to COVESA VSS paths from Quad simulator and republishes to custom data model paths.
"""

import sys
import os
import time
import logging
import threading
import json

# Add the SDK proto path
proto_path = '/workspaces/vehicle-app-cpp-template/vehicle-app-cpp-sdk-v1.0.3/proto'
if os.path.exists(proto_path):
    sys.path.insert(0, proto_path)

try:
    import grpc
    from google.protobuf import struct_pb2
    import duo.duo_pb2 as duo_pb2
    import duo.duo_pb2_grpc as duo_pb2_grpc
except ImportError as e:
    print(f"ERROR: Could not import Duo gRPC modules: {e}")
    print("Protobuf files may need to be compiled.")
    print(f"Run: python -m grpc_tools.protoc -I{proto_path} --python_out={proto_path} --grpc_python_out={proto_path} {proto_path}/duo/duo.proto")
    sys.exit(1)

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [VSS-ADAPTER] %(levelname)s: %(message)s'
)
logger = logging.getLogger(__name__)

# Path translation map: COVESA VSS path -> Custom data model path
PATH_TRANSLATION_MAP = {
    # Motion/Speed related - COVESA VSS path -> Custom data model path
    "Vehicle/Speed": "Vehicle/EgoVehicle/Motion/Locomotion/Speed",
    
    # Add more mappings as needed from your data model
    "Vehicle/CurrentLocation/Latitude": "Vehicle/EgoVehicle/General/State/CurrentLocation/Latitude",
    "Vehicle/CurrentLocation/Longitude": "Vehicle/EgoVehicle/General/State/CurrentLocation/Longitude",
    "Vehicle/CurrentLocation/Altitude": "Vehicle/EgoVehicle/General/State/CurrentLocation/Altitude",
}

# Reverse map for subscribing to COVESA paths
COVESA_PATHS = list(set([k.replace(".", "/") for k in PATH_TRANSLATION_MAP.keys()]))

class VSSPathAdapter:
    def __init__(self, broker_address: str = "127.0.0.1:50051", thing_name: str = "vss"):
        self.broker_address = broker_address
        self.thing_name = thing_name
        self.channel = None
        self.shadow_stub = None
        self.running = False
        
    def connect(self):
        """Connect to the Duo/Quad broker."""
        logger.info(f"Connecting to Duo broker at {self.broker_address}")
        self.channel = grpc.insecure_channel(self.broker_address)
        self.shadow_stub = duo_pb2_grpc.ShadowStub(self.channel)
        logger.info("Connected to Duo broker")
        
    def translate_path(self, covesa_path: str) -> str:
        """Translate COVESA VSS path to custom data model path."""
        # Normalize path (replace . with /)
        normalized = covesa_path.replace(".", "/")
        
        # Check if translation exists
        if normalized in PATH_TRANSLATION_MAP:
            return PATH_TRANSLATION_MAP[normalized]
        
        # Also check the dotted version
        dotted = covesa_path.replace("/", ".")
        if dotted in PATH_TRANSLATION_MAP:
            return PATH_TRANSLATION_MAP[dotted].replace(".", "/")
            
        # Return as-is if no translation found
        logger.debug(f"No translation found for {covesa_path}, using as-is")
        return normalized
        
    def subscribe_and_translate(self):
        """Subscribe to COVESA VSS shadow and translate to custom paths."""
        logger.info(f"Starting subscription to VSS shadow (thing: {self.thing_name})")
        
        try:
            # Create subscription request for VSS shadow
            request = duo_pb2.ListenReportShadowRequest(thing=self.thing_name)
            
            # Subscribe to shadow updates
            for response in self.shadow_stub.ListenReportShadow(request):
                if not self.running:
                    break
                    
                try:
                    # Extract the shadow data
                    if response.HasField('item'):
                        self.process_shadow_update(response.item)
                except Exception as e:
                    logger.error(f"Error processing shadow update: {e}")
                    
        except grpc.RpcError as e:
            logger.error(f"gRPC error in subscription: {e}")
        except Exception as e:
            logger.error(f"Unexpected error in subscription: {e}")
            
    def process_shadow_update(self, shadow_data):
        """Process shadow update and republish with translated paths."""
        try:
            # Convert protobuf struct to dict
            shadow_dict = self.protobuf_to_dict(shadow_data)
            
            # Extract VSS data and translate paths
            translated_data = {}
            self.extract_and_translate(shadow_dict, "", translated_data)
            
            if translated_data:
                # Republish with translated paths
                self.publish_translated_data(translated_data)
                
        except Exception as e:
            logger.error(f"Error processing shadow data: {e}")
            
    def extract_and_translate(self, data, current_path: str, result: dict):
        """Recursively extract leaf values and translate paths."""
        if isinstance(data, dict):
            for key, value in data.items():
                new_path = f"{current_path}/{key}" if current_path else key
                self.extract_and_translate(value, new_path, result)
        elif not isinstance(data, (list, tuple)):
            # Leaf value found
            if current_path:
                # Translate the path
                translated_path = self.translate_path(current_path)
                if translated_path != current_path:
                    logger.info(f"Translating: {current_path} -> {translated_path} = {data}")
                    result[translated_path] = data
                
    def protobuf_to_dict(self, proto_struct) -> dict:
        """Convert protobuf Struct to Python dict."""
        from google.protobuf.json_format import MessageToDict
        return MessageToDict(proto_struct, preserving_proto_field_name=True)
        
    def publish_translated_data(self, data: dict):
        """Publish translated data back to the shadow."""
        try:
            # Build the nested structure for translated paths
            shadow_update = {}
            
            for path, value in data.items():
                parts = path.split("/")
                current = shadow_update
                
                for i, part in enumerate(parts[:-1]):
                    if part not in current:
                        current[part] = {}
                    current = current[part]
                    
                # Set the leaf value
                current[parts[-1]] = value
                
            # Create patch request
            from google.protobuf.struct_pb2 import Struct
            item = Struct()
            self.dict_to_protobuf(shadow_update, item)
            
            request = duo_pb2.PatchReportShadowRequest(
                thing=self.thing_name,
                item=item
            )
            
            # Send the patch
            response = self.shadow_stub.PatchReportShadow(request)
            logger.info(f"Published translated data for paths: {list(data.keys())}")
            
        except Exception as e:
            logger.error(f"Error publishing translated data: {e}")
            
    def dict_to_protobuf(self, data: dict, proto_struct):
        """Convert Python dict to protobuf Struct."""
        from google.protobuf.json_format import ParseDict
        ParseDict(data, proto_struct)
        
    def start(self):
        """Start the adapter service."""
        self.running = True
        self.connect()
        
        logger.info("VSS Path Adapter service started")
        logger.info(f"Translation map loaded with {len(PATH_TRANSLATION_MAP)} entries")
        
        # Start subscription in a separate thread
        subscribe_thread = threading.Thread(target=self.subscribe_and_translate, daemon=True)
        subscribe_thread.start()
        
        # Keep the main thread alive
        try:
            while self.running:
                time.sleep(1)
        except KeyboardInterrupt:
            logger.info("Shutting down adapter...")
            self.stop()
            
    def stop(self):
        """Stop the adapter service."""
        self.running = False
        if self.channel:
            self.channel.close()
        logger.info("VSS Path Adapter stopped")

def main():
    import os
    
    # Get broker address from environment or use default
    broker_address = os.getenv("DUO_BROKER_ADDRESS", "127.0.0.1:50051")
    thing_name = os.getenv("VSS_THING_NAME", "vss")
    
    adapter = VSSPathAdapter(broker_address=broker_address, thing_name=thing_name)
    adapter.start()

if __name__ == "__main__":
    main()
