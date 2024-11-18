from PIL import Image
def txt_to_image(txt_file_path, output_image_path):
    # Set the known size of the image
    width = 320
    height = 240

    print(f"Image dimensions: {height}x{width}")

    try:
        # Open the file and check if it has enough data
        with open(txt_file_path, 'rb') as file:
            file_data = file.read()

        # Create a new image with white background
        image = Image.new('1', (width, height), 1)

        # Load the pixel map
        pixels = image.load()

        # Initialize coordinates
        x, y = 0, 0

        # Process the image byte by byte
        for i, byte in enumerate(file_data):
            if byte == ord(' '):  # White pixel
                pixels[x, y] = 0
            elif byte == ord('*'):  # Black pixel
                pixels[x, y] = 1
            else:
                continue

            # Move to the next pixel
            x += 1
            if x >= width:  # End of row, move to the next
                x = 0
                y += 1
                if y >= height:  # Stop if we exceed the image height
                    break

        # Save the image
        image.save(output_image_path)
        print(f"Image saved to {output_image_path}")

    except Exception as e:
        print(f"Error: {e}")

txt_to_image('./fpga_return/img_r_channel.txt', './fpga_return/victory.png')