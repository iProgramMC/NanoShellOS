using System.IO;
using System.Drawing;
using System.Text;

if (args.Length < 3)
{
    Console.WriteLine("Usage: fontconv <bitmapfile> <lengthfile> <output_name>");
}

string bmp = args[0], len = args[1], output = args[2], outputFile = args[2]+".h";
byte[] lengthBytes = File.ReadAllBytes(len);
Bitmap image = new("font.png");
int cWidth = image.Width / 16;
int cHeight = image.Height / 16;

StringBuilder pout = new();
pout.Append("/*****************************************\n");
pout.Append("        NanoShell Operating System\n");
pout.Append("       (c) 2021 -2022 iProgramInCpp\n\n");
pout.Append(" This is a converted asset to embed into\n");
pout.Append("            the kernel image.\n");
pout.Append($" * Converted File: {bmp}\n");
pout.Append($" * Converted Date: {DateTime.Now.ToShortDateString()}\n");
pout.Append("*****************************************/\n\n");

//print file data header:
pout.Append($"const unsigned char g_{output}_data[] = {{\n");
{
    //print character width and height:
    pout.Append($"\t{cWidth}, {cHeight},\n\t");
    pout.Append("//character data:\n\t");
    for (int i = 0; i < 256; i++)
    {
        int cx = i % 16, cy = i / 16;
        cx *= cWidth; cy *= cHeight;
        for (int j = 0; j < cHeight; j++)
        {
            int data = 0;
            for (int k = 0; k < cWidth; k++)
            {
                if (image.GetPixel(cx + k, cy + j).A == 255)
                    data |= (1 << (cWidth-1 - k));
            }
            pout.Append($"0x{data:X2},");
        }
        pout.Append("\n\t");
    }
    pout.Append("//length data:");
    for (int i = 0; i < 256; i++)
    {
        if (i % 16 == 0)
        {
            pout.Append("\n\t");
        }
        pout.Append($"0x{lengthBytes[i]:X2},");
    }
}
pout.Append("\n};\n\n");

File.WriteAllText(outputFile, pout.ToString());


