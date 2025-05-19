#ifndef SAFETY_CRC_H
#define SAFETY_CRC_H

/*!
  \file
  \~japanese
  \brief 安全センサーのCRC計算

  安全センサー用の基本的なCRC計算に関する関数を提供します。

  \~english
  \brief Safety sensor CRC

  Provides the basic functions for Safety sensor's CRC calculation

 $Id$
*/

#ifdef __cplusplus
extern "C" {
#endif

	    /*!
      \~japanese
      \brief CRCテーブルを初期化する

      \param[in] なし

      \retval なし

      \~english
      \brief CRC table initialization

      \param[in] None

      \retval None
    */
	void safety_init_crc();

	    /*!
      \~japanese
      \brief CRCの計算

      \param[in] data 文字列
      \param[in] size data の byte サイズ

      \retval CRCの計算後の数値

      \~english
      \brief Claculates the CRC code of a message

      \param[in] data the message to calculate its CRC
      \param[in] size of the data

      \retval CRC value
    */
	unsigned short safety_calc_crc(const char *src, int length);

#ifdef __cplusplus
}
#endif

#endif /* !SAFETY_CRC_H */