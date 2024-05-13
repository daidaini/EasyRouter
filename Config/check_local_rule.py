#!/usr/bin/python3

#字典映射，模块对应的值固定不变
g_GwTypeDict = {
    'HST2': 1, 
    'HST3': 2,
    'DD20': 3, 
    'DDA5': 4, 
    'HTS_Option': 5, 
    'HTS_Stock': 6, 
    'JSD': 7, 
    'JSD_Stock' : 8
}

def check_account(account : str) :
    if account.startswith("10") :
        return g_GwTypeDict['HST2']
    elif account.startswith("00"):
        return g_GwTypeDict['JSD_Stock']
    else:
        return g_GwTypeDict['DDA5']



if __name__ == '__main__' :
    print("result = ", check_account('101302822'))

    print("result = ", check_account('0020045846'))

    print("result = ", check_account('015000055035'))