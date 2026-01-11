using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Hexconverters
{
    public class Hexstring
    {
        /// <summary>
        /// 将字符串转换为字节数组
        /// </summary>
        /// <param name="str">要转换的字符串</param>
        /// <param name="len">字节数组的长度，默认为8</param>
        /// <returns>转换后的字节数组</returns>
        public static byte[] GetBytes(string str, int len)
        {
            byte[] bytes = new byte[len];
            if (string.IsNullOrEmpty(str))
                return bytes;
            string[] strArr = str.Trim().Split(' ');
            for (int i = 0; i < len; i++)
            {
                var item = strArr[i];
                bytes[i] = Convert.ToByte(item, 16);
            }
            return bytes;
        }

        /// <summary>
        /// 将字符串转换为字节数组
        /// </summary>
        /// <param name="str">要转换的字符串</param>
        /// <returns>转换后的字节数组</returns>
        public static byte[] GetBytes(string str)
        {
            if (string.IsNullOrEmpty(str))
                return null;
            string[] strArr = str.Trim().Split(' ');
            byte[] bytes = new byte[strArr.Length];
            for (int i = 0; i < strArr.Length; i++)
            {
                var item = strArr[i];
                try
                {
                    bytes[i] = Convert.ToByte(item, 16);
                }
                catch (Exception e)
                {
                    throw e;
                }
            }
            return bytes;
        }

        /// <summary>
        /// 将字节数组转换为字符串
        /// </summary>
        /// <param name="vs">要转换的字节数组</param>
        /// <param name="len">要转换的长度</param>
        /// <returns>转换后的字符串</returns>
        public static string ToString(byte[] vs, int len)
        {
            string str = String.Empty;
            if (vs == null)
                return str;
            if (len > vs.Length)
                len = vs.Length;
            for (int i = 0; i < len; i++)
                str += vs[i].ToString("X2") + " ";
            return str.Trim();
        }

        /// <summary>
        /// 将字节数组转换为字符串
        /// </summary>
        /// <param name="vs">要转换的字节数组</param>
        /// <returns>转换后的字符串</returns>
        public static string ToString(byte[] vs)
        {
            string str = String.Empty;
            if (vs == null)
                return str;
            for (int i = 0; i < vs.Length; i++)
                str += vs[i].ToString("X2") + " ";
            return str.Trim();
        }
    }
}
