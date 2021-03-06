/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright (c) 2010, LABUST, UNIZG-FER
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the LABUST nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/
#ifndef STRINGUTILITIES_HPP_
#define STRINGUTILITIES_HPP_
#include <string>
#include <sstream>

namespace labust
{
  namespace tools
  {
    /**
     * The function split the string into two parts separated by a delimiter.
     *
     * \param main The string that will be split.
     * \param token Split delimiter character.
     * \return The string before the delimiter.
     */
    inline std::string chomp(std::string& main, const char token = ',')
    {
      int pos=int(main.find(token));
      std::string part=main.substr(0,pos);
      main.erase(0,pos + (pos != -1));
      return part;
    }
    /**
     * The function converts different types into string if
     * a output operator is defined for the type.
     *
     * \param value Value which to convert into a string.
     *
     * \tparam MyType Type of the converted object.
     *
     * \return Returns the converted object in a string.
     */
    template <class MyType>
    inline std::string to_string(MyType value)
    {
      std::stringstream out;
      out.precision(6);
      out<<std::fixed<<value;
      return out.str();
    }
    /**
     * The function returns the checksum of a byte array.
     *
     * \param data Address of the range start.
		 * \param len Length of the range.
		 *
		 * \return XOR checksum of the given range.
     */
    inline unsigned char getChecksum(const unsigned char* const data, size_t len)
    {
      unsigned char calc = 0;
      for (size_t i=0; i<len; ++i){calc^=data[i];};
      return calc;
    }
  }
}
/* STRINGUTILITIES_HPP_ */
#endif
