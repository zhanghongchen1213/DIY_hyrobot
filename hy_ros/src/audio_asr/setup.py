from setuptools import find_packages, setup

package_name = 'audio_asr'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        # 添加launch文件配置
        ('share/' + package_name + '/launch', ['launch/audio_asr.launch.py']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='k',
    maintainer_email='k@todo.todo',
    description='Streaming ASR over sherpa-onnx with rknn provider',
    license='Apache-2.0',
    extras_require={
        'test': [
            'pytest',
        ],
    },
    entry_points={
        'console_scripts': [
            'audio_asr_node = audio_asr.main:main',
        ],
    },
)
