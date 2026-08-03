"""
RMAGC — 4-Axis Robot Arm Kinematics Simulation

DH parameter based forward and inverse kinematics with 3D visualization.
Size-agnostic: change L1, L2, L3, H0 to match any 4-axis arm geometry.

Usage:
    python kinematics.py
"""

import numpy as np
from scipy.optimize import minimize
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D


class FourAxisArm:
    """
    Generic 4-axis robot arm (base rotation + planar 3R).

    DH parameters: [a, alpha, d, theta] for each joint.
    theta is the joint variable; a, alpha, d are fixed geometry.
    """

    def __init__(self):
        # ---------- Adjust these to match your actual arm ----------
        L1 = 100.0   # Upper arm length (mm) — shoulder → elbow
        L2 = 120.0   # Forearm length (mm) — elbow → end effector
        L3 = 0.0     # Wrist offset (mm) — set > 0 if gripper has length
        H0 = 25.0    # Base height (mm) — rotation center above table

        self.a = [0.0, L1, L2, L3]
        self.alpha = [0.0, 0.0, 0.0, 0.0]  # All parallel axes (planar)
        self.d = [H0, 0.0, 0.0, 0.0]

    def dh_transform(self, a, alpha, d, theta):
        """Single DH transformation matrix."""
        ct = np.cos(theta)
        st = np.sin(theta)
        ca = np.cos(alpha)
        sa = np.sin(alpha)
        return np.array([
            [ct, -st * ca,  st * sa, a * ct],
            [st,  ct * ca, -ct * sa, a * st],
            [0,       sa,      ca,      d],
            [0,        0,       0,      1],
        ])

    def forward(self, joints):
        """
        Forward kinematics.

        Args:
            joints: [theta1, theta2, theta3, theta4] in radians

        Returns:
            End effector position [x, y, z] in mm
        """
        T = np.eye(4)
        for i in range(4):
            Ti = self.dh_transform(self.a[i], self.alpha[i],
                                   self.d[i], joints[i])
            T = T @ Ti
        return T[:3, 3]

    def inverse(self, target_pos, initial_guess=None):
        """
        Numerical inverse kinematics using optimization.

        Args:
            target_pos: [x, y, z] in mm
            initial_guess: starting joint angles in radians

        Returns:
            Joint angles [theta1, theta2, theta3, theta4] in radians
        """
        if initial_guess is None:
            initial_guess = [0.0, np.pi / 4, -np.pi / 4, 0.0]

        def error(joints):
            pos = self.forward(joints)
            return np.linalg.norm(pos - target_pos)

        result = minimize(
            error,
            initial_guess,
            method='SLSQP',
            bounds=[(-np.pi, np.pi)] * 4,
        )

        if result.success:
            return result.x
        else:
            raise RuntimeError(f"IK failed: {result.message}")

    def get_joint_positions(self, joints):
        """Return [x,y,z] of each joint (base → end effector) for plotting."""
        T = np.eye(4)
        points = [np.array([0.0, 0.0, 0.0])]
        for i in range(4):
            Ti = self.dh_transform(self.a[i], self.alpha[i],
                                   self.d[i], joints[i])
            T = T @ Ti
            points.append(T[:3, 3])
        return np.array(points)


def plot_arm(arm, joints, ax, color='steelblue', label=None):
    """Draw the arm as connected line segments in 3D."""
    points = arm.get_joint_positions(joints)
    ax.plot(points[:, 0], points[:, 1], points[:, 2],
            'o-', linewidth=2.5, markersize=6, color=color, label=label)
    ax.scatter(*points[-1], color='red', s=60, zorder=5)  # End effector


def main():
    arm = FourAxisArm()

    # --- Test forward kinematics ---
    test_joints = [0.2, 0.5, -0.8, 0.3]
    pos = arm.forward(test_joints)
    print(f"Forward  — joints: {np.round(test_joints, 3)}")
    print(f"            pos:   {np.round(pos, 1)} mm")

    # --- Test inverse kinematics ---
    solved = arm.inverse(pos, initial_guess=test_joints)
    verified = arm.forward(solved)
    error = np.linalg.norm(verified - pos)
    print(f"Inverse   — solved joints: {np.round(solved, 3)}")
    print(f"            verified pos:   {np.round(verified, 1)} mm")
    print(f"            error:          {error:.3f} mm")

    # --- 3D Visualization ---
    fig = plt.figure(figsize=(10, 8))
    ax = fig.add_subplot(111, projection='3d')

    # Original arm
    plot_arm(arm, test_joints, ax, color='steelblue', label='Original')

    # Solved arm (should overlap if IK is correct)
    plot_arm(arm, solved, ax, color='darkorange', label='IK Solved')

    # Target marker
    ax.scatter(*pos, color='green', s=100, marker='*', zorder=10, label='Target')

    # Workspace bounds
    ax.set_xlim([-300, 300])
    ax.set_ylim([-300, 300])
    ax.set_zlim([0, 400])
    ax.set_xlabel('X (mm)')
    ax.set_ylabel('Y (mm)')
    ax.set_zlabel('Z (mm)')
    ax.set_title('RMAGC — 4-Axis Arm Kinematics')
    ax.legend()
    plt.tight_layout()
    plt.show()


if __name__ == "__main__":
    main()
