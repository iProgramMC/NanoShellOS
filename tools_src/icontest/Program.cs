using System;
using System.IO;
using System.Drawing;
using System.Text;

namespace icontest
{
    internal class Program
    {
        static void Main(string[] args)
        {
            if (args.Length < 2)
            {
                Console.WriteLine("Usage: iconconv <bitmapfile> <output_name>");
                return;
            }

            string bmp = args[0], output = args[1], outputFile = args[1];
            if (output.EndsWith(".h"))
                output = output.Substring(0, output.Length - 2);
            int li = output.LastIndexOf('/');
            if (li != -1)
                output = output.Substring(li+1);

            Bitmap image = new Bitmap(bmp);

            StringBuilder pout = new StringBuilder(5000);
            pout.Append("/*****************************************\n");
            pout.Append("        NanoShell Operating System\n");
            pout.Append("          (c) 2022 iProgramInCpp\n\n");
            pout.Append("  This is a converted icon to embed into\n");
            pout.Append("            the kernel image.\n");
            pout.Append($" * Converted File: {bmp}\n");
            pout.Append($" * Converted Date: {DateTime.Now.ToShortDateString()}\n");
            pout.Append($" * Icon Last Mod:  {File.GetLastWriteTime(bmp).ToShortDateString()}\n");
            pout.Append("*****************************************/\n\n");

            //print file data header:
            pout.Append($"const uint32_t g_{output}_data[] = {{\n");
            {
                //print the RGBA
                for (int y = 0; y < image.Height; y++)
                {
                    pout.Append("\t");
                    for (int x = 0; x < image.Width; x++)
                    {
                        uint rgba = 0;
                        var pixel = image.GetPixel(x, y);
                        rgba |= (uint)(pixel.B << 0);
                        rgba |= (uint)(pixel.G << 8);
                        rgba |= (uint)(pixel.R << 16);
                        if (pixel.A == 0)
                        {
                            rgba = 0xFFFFFFFF;//transparent
                        }
                        pout.Append($"0x{rgba:X8},");
                    }
                    pout.Append('\n');
                }
            }
            pout.Append("};\n\n");

            pout.Append($"Image g_{output}_icon = {{\n");
            {
                //image width and height:
                pout.Append($"\t{image.Width}, {image.Height}, g_{output}_data\n");
            }
            pout.Append("};\n");
            File.WriteAllText(outputFile, pout.ToString());
        }
    }
}
