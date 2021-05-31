#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "encr_decr_block.h"
#define MATRIX_ROWS_COLUMNS 4

// Operation xtime represents multiplying by 2 in Mix Columns operations
// Left side is multiplying by 2, right side is adding 0x1b by mod2, if number is higher than 127
#define xtime(value)                            \
        ((value<<1) ^ (((value>>7) & 1) * 0x1b)) \

// Multiplying here where made by representing numbers of InvMixColumns matrix as powers of 2
// For using xtime() operation several times to multiply (xtime represents multiplying by 2)
// For more information: https://link.springer.com/chapter/10.1007%2F978-3-662-04722-4_4
#define multiply(value, value_from_const_martix)                                \
      ((value_from_const_martix & 1) * value) ^                                  \
      ((value_from_const_martix>>1 & 1) * xtime(value)) ^                         \
      ((value_from_const_martix>>2 & 1) * xtime(xtime(value))) ^                   \
      ((value_from_const_martix>>3 & 1) * xtime(xtime(xtime(value))))               \
       
// S-box for encryption
static const uint8_t Sbox[256] =
{
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

// Inverted S-box for decrypting
static const uint8_t InvSbox[256] =
{
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
    0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
    0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
    0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
    0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
    0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
    0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
    0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
    0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
    0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
    0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
};

// Rcon vector for KeyShedule
static const uint8_t Rcon[10] =
{
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
};

void AddRoundKey(uint8_t state[4][4], uint8_t RoundKey[4][4])
{
    // Moving on columns
    for (int index_columns = 0; index_columns < 4; index_columns++)
    {
        for (int index_strings = 0; index_strings < 4; index_strings++)
        {
            // Addition by mod 2 using bit operations
            state[index_strings][index_columns] = RoundKey[index_strings][index_columns] ^ state[index_strings][index_columns];
        }
    }
}

void SubBytes(uint8_t state[4][4])
{
    // Moving on columns
    for (int index_columns = 0; index_columns < 4; index_columns++)
    {
        for (int index_strings = 0; index_strings < 4; index_strings++)
        {
            // Getting numbers from state matrix and putting number from Sbox to it instead
            state[index_strings][index_columns] = Sbox[state[index_strings][index_columns]];
        }
    }
}

void Shift_One_Row(uint8_t state[4][4], int num_of_shifts, int index_of_row)
{
    // Num_of_shifts == index_of_row, its here to make code more readable
    uint8_t saved_value;
    while (num_of_shifts--)
    {
        // Code for cyclic shifting one raw to the left
        saved_value = state[index_of_row][0];
        for (int i = 0; i < MATRIX_ROWS_COLUMNS - 1; i++)
        {
            state[index_of_row][i] = state[index_of_row][i + 1];
        }
        state[index_of_row][MATRIX_ROWS_COLUMNS - 1] = saved_value;
    }
}

void ShiftRows(uint8_t state[4][4])
{
    for (int j = 1; j < 4; j++)
    {
        Shift_One_Row(state, j, j);
    }
}

void MixColumns(uint8_t state[4][4])
{
    // if multiplying by 1 just leaving the number
    // if multiplying by 2 doing xtime(num)
    // if multiplying by 3 doing xtime(num)^num
    uint8_t first, second, third, fourth;
    for (int i = 0; i < 4; i++)
    {
        // Elements of column
        first = state[0][i];
        second = state[1][i];
        third = state[2][i];
        fourth = state[3][i];
        state[0][i] = xtime(first) ^ (xtime(second) ^ second) ^ third ^ fourth;
        state[1][i] = first ^ xtime(second) ^ (xtime(third) ^ third) ^ fourth;
        state[2][i] = first ^ second ^ xtime(third) ^ (xtime(fourth) ^ fourth);
        state[3][i] = (xtime(first)^first) ^ second ^ third ^ xtime(fourth);
    }
}

void KeyShedule(uint8_t RoundKey[4][4], int key_shedule_index)
{
    uint8_t last_column[4];
    uint8_t saved_value;
    // Saving last column
    for (int i = 0; i < 4; i++)
    {
        last_column[i] = RoundKey[i][3];
    }
    saved_value = last_column[0];
    // Shifting column
    for (int i = 0; i < MATRIX_ROWS_COLUMNS - 1; i++)
    {
        last_column[i] = last_column[i + 1];
    }
    last_column[3] = saved_value;
    // SubBytes of column
    for (int i = 0; i < 4; i++)
    {
        last_column[i] = Sbox[last_column[i]];
    }
    // Adding by mod 2
    last_column[0] = RoundKey[0][0] ^ last_column[0] ^ Rcon[key_shedule_index];
    for (int i = 1; i < 4; i++)
    {
        last_column[i] = RoundKey[i][0] ^ last_column[i];
    }
    // Got first column for new key, rewriting current key first column
    for (int i = 0; i < 4; i++)
    {
        RoundKey[i][0] = last_column[i];
    }
    // Rewriting whole key by columns by mod 2 (Next column = that column + previous column by mod 2)
    for (int i_column = 1; i_column < 4; i_column++)
    {
        for (int i_string = 0; i_string < 4; i_string++)
        {
            RoundKey[i_string][i_column] = RoundKey[i_string][i_column] ^ RoundKey[i_string][i_column - 1];
        }
    }
}

void encrypting_block(uint8_t state[4][4], uint8_t key[4][4])
{
    uint8_t working_key[4][4];
    // Coping key to working_key which will change in function
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            working_key[i][j] = key[i][j];
        }
    }
    // First adding key
    AddRoundKey(state, working_key);
    // Making 9 rounds
    for (int i = 0; i < 9; i++)
    {
        SubBytes(state);
        ShiftRows(state);
        MixColumns(state);
        KeyShedule(working_key, i);
        AddRoundKey(state, working_key);
    }
    // Making final round
    SubBytes(state);
    ShiftRows(state);
    KeyShedule(working_key, 9);
    AddRoundKey(state, working_key);
}

//=================================================================================================
// Decrypting process
void InvSubBytes(uint8_t state[4][4])
{
    // Moving on columns
    for (int index_columns = 0; index_columns < 4; index_columns++)
    {
        for (int index_strings = 0; index_strings < 4; index_strings++)
        {
            // Getting numbers from state matrix and putting number from Sbox to it instead
            state[index_strings][index_columns] = InvSbox[state[index_strings][index_columns]];
        }
    }
}


void InvShift_One_Row(uint8_t state[4][4], int num_of_shifts, int index_of_row)
{
    uint8_t saved_value;
    while (num_of_shifts--)
    {
        // Code for cyclic shifting one row to the right
        saved_value = state[index_of_row][MATRIX_ROWS_COLUMNS - 1];
        for (int i = MATRIX_ROWS_COLUMNS - 1; i > 0; i--)
        {
            state[index_of_row][i] = state[index_of_row][i - 1];
        }
        state[index_of_row][0] = saved_value;
    }
}

void InvShiftRows(uint8_t state[4][4])
{
    for (int j = 1; j < 4; j++)
    {
        InvShift_One_Row(state, j, j);
    }
}

void InvMixColumns(uint8_t state[4][4])
{
    uint8_t first, second, third, fourth;
    for (int i = 0; i < 4; i++)
    {
        // Elements of column
        first = state[0][i];
        second = state[1][i];
        third = state[2][i];
        fourth = state[3][i];
        // Multiplying column to InvMixColumns matrix
        state[0][i] = multiply(first, 0x0e) ^ multiply(second, 0x0b) ^ multiply(third, 0x0d) ^ multiply(fourth, 0x09);
        state[1][i] = multiply(first, 0x09) ^ multiply(second, 0x0e) ^ multiply(third, 0x0b) ^ multiply(fourth, 0x0d);
        state[2][i] = multiply(first, 0x0d) ^ multiply(second, 0x09) ^ multiply(third, 0x0e) ^ multiply(fourth, 0x0b);
        state[3][i] = multiply(first, 0x0b) ^ multiply(second, 0x0d) ^ multiply(third, 0x09) ^ multiply(fourth, 0x0e);
    }
}

void KeyShedule_decryption(uint8_t key[4][4], uint8_t output_key[4][4], int index_shedule)
{
    // Coping key to output_key
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            output_key[i][j] = key[i][j];
        }
    }
    for (int i = 0; i < index_shedule + 1; i++)
    {
        KeyShedule(output_key, i);
    }
}

void decrypting_block(uint8_t encrypted_message[4][4], uint8_t key[4][4])
{
    // State is changing and we can rewrite it, but key should be the same
    // due to KeyShedule, which is based on previous versions
    // Process of decrypting is inverted to process of encrypting
    uint8_t current_key[4][4];
    KeyShedule_decryption(key, current_key, 9);
    AddRoundKey(encrypted_message, current_key);
    InvShiftRows(encrypted_message);
    InvSubBytes(encrypted_message);
    for (int i = 8; i > -1; i--)
    {
        KeyShedule_decryption(key, current_key, i);
        AddRoundKey(encrypted_message, current_key);
        InvMixColumns(encrypted_message);
        InvShiftRows(encrypted_message);
        InvSubBytes(encrypted_message);
    }
    AddRoundKey(encrypted_message, key);
}




