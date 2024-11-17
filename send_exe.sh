# Check if the user provided an IP address
if [ -z "$1" ] || [ -z "$2" ]; then
  echo "Usage: $(basename $0) <IP_ADDRESS> <FILE_NAME>"
  return 1 2>/dev/null
fi

# Set the IP address from the first argument
IP_ADDRESS=$1
FILE_NAME=$2

# Use the provided IP address in the scp command
scp $FILE_NAME root@$IP_ADDRESS:/home/root

# Optional: Print a success message
if [ $? -eq 0 ]; then
  echo "File successfully sent to $IP_ADDRESS"
else
  echo "Error occurred while sending the file."
fi