import time

import numpy as np
import pandas as pd

np.random.seed(2)

N_STATE = 6  # 距离宝藏有6步的距离
ACTION = ['left', 'right']  # 可选的动作方式
EPSILON = 0.9  # 选择率(有90%的概率选择最优的动作，10%的概率选择随机的动作)
ALPHA = 0.1  # 学习效率
LAMBDA = 0.9  #
MAX_EPISODES = 13  # 最大回合数(最大训练次数)
FRESH_TIME = 0.02  # 每一步的刷新时间


def build_q_table(n_states, actions):  # create a form
    table = pd.DataFrame(
        np.zeros((n_states, len(actions))), columns=actions)
    return table


def choose_action(state, q_table):
    state_actions = q_table.iloc[state, :]
    if (np.random.uniform() > EPSILON) or (state_actions.all() == 0):
        action_name = np.random.choice(ACTION)
    else:
        action_name = state_actions.argmax()
    return action_name


def get_env_feedback(S, A):
    if A == 'right':
        if S == N_STATE - 2:
            S_ = 'terminal'
            R = 1
        else:
            S_ = S + 1
            R = 0
    else:
        R = 0
        if S == 0:
            S_ = S
        else:
            S_ = S - 1
    return S_, R


def update_env(S, episode, step_counter):  # create the environment
    env_list = ['-'] * (N_STATE - 1) + ['T']
    if S == 'terminal':
        interaction = 'Episode %s: total_steps=%s' % (episode + 1, step_counter)
        print('\r{}'.format(interaction), end='')
        time.sleep(0.1)
        print('\r')
    else:
        env_list[S] = 'o'
        interaction = ''.join(env_list)
        print('\r{}'.format(interaction), end='')
        time.sleep(0.1)


def rl():
    q_table = build_q_table(N_STATE, ACTION)
    for episode in range(MAX_EPISODES):
        step_counter = 0
        S = 0
        is_terminated = False
        update_env(S, episode, step_counter)
        while not is_terminated:
            A = choose_action(S, q_table)
            S_, R = get_env_feedback(S, A)
            q_predict = q_table.loc[S, A]
            if S_ != 'terminal':
                q_target = R + LAMBDA * q_table.loc[S_, :].max()
            else:
                q_target = R
                is_terminated = True
            q_table.loc[S, A] += ALPHA * (q_target - q_predict)
            S = S_
            update_env(S, episode, step_counter + 1)
            step_counter += 1
    return q_table


if __name__ == '__main__':
    q_table = rl()
    print('\r\nQ-table:\n')
    print(q_table)
