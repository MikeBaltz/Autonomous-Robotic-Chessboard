.PHONY: flash bridge all

all: flash bridge

flash:
	@echo "Flashing movement sketch..."
	@pio run -t upload -d sketches/movement

bridge:
	@echo "Starting chess bridge..."
	@python3 software/chess_bridge/main.py

test-knight:
	@echo "Testing knight move (f3 to d4)..."
	@echo "f3d4" | python3 software/chess_bridge/main.py
