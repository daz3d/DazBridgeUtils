#pragma once

#include <QImage>
#include <QColor>
#include <QThreadPool>
#include <QRunnable>

class MultiplyImageByColorTask : public QRunnable
{
public:
	MultiplyImageByColorTask(QImage* image, QColor color, int startY, int endY, int debug_width, int debug_height, QImage::Format debug_format)
		: m_image(image), m_color(color), m_startY(startY), m_endY(endY), m_expectedWidth(debug_width), m_expectedHeight(debug_height), m_pixelFormat(debug_format)
	{
	}

	void run()
	{
		int width = m_image->width();
		QRgb* scanLine;
		for (int y = m_startY; y < m_endY; ++y)
		{
			scanLine = reinterpret_cast<QRgb*>(m_image->scanLine(y));
			for (int x = 0; x < width; ++x)
			{
				QRgb pixel = scanLine[x];
				int red = qRed(pixel);
				int green = qGreen(pixel);
				int blue = qBlue(pixel);

				int newRed = qMin(255, red * m_color.red() / 255);
				int newGreen = qMin(255, green * m_color.green() / 255);
				int newBlue = qMin(255, blue * m_color.blue() / 255);

				scanLine[x] = qRgb(newRed, newGreen, newBlue);
			}
		}
	}

private:
	QImage* m_image;
	QColor m_color;
	int m_startY;
	int m_endY;
	// Crash Checks
	int m_expectedWidth;
	int m_expectedHeight;
	QImage::Format m_pixelFormat;
};

class MultiplyImageByStrengthTask : public QRunnable
{
public:
	MultiplyImageByStrengthTask(QImage* image, double strength, int startY, int endY, int debug_width, int debug_height, QImage::Format debug_format)
		: m_image(image), m_strength(strength), m_startY(startY), m_endY(endY), m_expectedWidth(debug_width), m_expectedHeight(debug_height), m_pixelFormat(debug_format)
	{
	}

	void run()
	{
		int width = m_image->width();
		QRgb* scanLine;
		for (int y = m_startY; y < m_endY; ++y)
		{
			scanLine = reinterpret_cast<QRgb*>(m_image->scanLine(y));
			for (int x = 0; x < width; ++x)
			{
				QRgb pixel = scanLine[x];
				int red = qRed(pixel);
				int green = qGreen(pixel);
				int blue = qBlue(pixel);

				int newRed = qMin(255, (int) (red * m_strength / 255) );
				int newGreen = qMin(255, (int) (green * m_strength / 255) );
				int newBlue = qMin(255, (int) (blue * m_strength / 255) );

				scanLine[x] = qRgb(newRed, newGreen, newBlue);
			}
		}
	}

private:
	QImage* m_image;
	double m_strength;
	int m_startY;
	int m_endY;
	// Crash Checks
	int m_expectedWidth;
	int m_expectedHeight;
	QImage::Format m_pixelFormat;
};

class ImageTools
{
public:
	static void multiplyImageByColorMultithreaded(QImage& image, QColor color);
	static void multiplyImageByStrengthMultithreaded(QImage& image, double strength);
	static QString colorToHexString(const QColor& color);
	static bool isPowerOfTwo(int n);
	static int nearestPowerOfTwo(int n);

};
