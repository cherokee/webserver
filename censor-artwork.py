import os

def main():
	for root, dirs, files in os.walk('.'):
		for filename in files:
			fp = os.path.join (root, filename)
			if '-CENSORED.' in filename:
				print(f"cp {fp} {fp.replace('-CENSORED', '')}")

if __name__ == "__main__":
	main()
