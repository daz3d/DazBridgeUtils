#pragma once

#include <QImage>
#include <QColor>
#include <QThreadPool>
#include <QRunnable>

class ImageTools
{
public:
	static void multiplyImageByColorMultithreaded(QImage& image, QColor color);
	static void multiplyImageByStrengthMultithreaded(QImage& image, double strength);
	static QString colorToHexString(const QColor& color);
	static bool isPowerOfTwo(int n);
	static int nearestPowerOfTwo(int n);
	static void BlendImagesWithAlphaMultithreaded(QImage& imageA, const QImage& imageB, const QImage& alphaMask);
	static void ConvertImageLinearToRgbMultithreaded(QImage& image);

};

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

class BlendImagesWithAlphaTask : public QRunnable
{
public:
	BlendImagesWithAlphaTask(QImage* imageA, const QImage* imageB, const QImage* alphaMask, int startY, int endY)
		: m_imageA(imageA), m_imageB(imageB), m_alphaMask(alphaMask), m_startY(startY), m_endY(endY)
	{
	}

	void run() override
	{
		int width = m_imageA->width();
		for (int y = m_startY; y < m_endY; ++y)
		{
			QRgb* scanLineA = reinterpret_cast<QRgb*>(m_imageA->scanLine(y));
			const QRgb* scanLineB = reinterpret_cast<const QRgb*>(m_imageB->constScanLine(y));
			const QRgb* scanLineAlpha = reinterpret_cast<const QRgb*>(m_alphaMask->constScanLine(y));

			for (int x = 0; x < width; ++x)
			{
				QRgb pixelA = scanLineA[x];
				QRgb pixelB = scanLineB[x];
				QRgb pixelAlpha = scanLineAlpha[x];

				int alpha = qRed(pixelAlpha); // Assuming grayscale alpha mask
				double alphaF = alpha / 255.0;
				double invAlphaF = 1.0 - alphaF;

				int r = qBound(0, static_cast<int>(qRed(pixelA) * invAlphaF + qRed(pixelB) * alphaF), 255);
				int g = qBound(0, static_cast<int>(qGreen(pixelA) * invAlphaF + qGreen(pixelB) * alphaF), 255);
				int b = qBound(0, static_cast<int>(qBlue(pixelA) * invAlphaF + qBlue(pixelB) * alphaF), 255);
				int a = qBound(0, static_cast<int>(qAlpha(pixelA) * invAlphaF + qAlpha(pixelB) * alphaF), 255);

				scanLineA[x] = qRgba(r, g, b, a);
			}
		}
	}

private:
	QImage* m_imageA;
	const QImage* m_imageB;
	const QImage* m_alphaMask;
	int m_startY;
	int m_endY;
};

class ConvertImageLinearToRgb : public QRunnable
{
public:
	ConvertImageLinearToRgb(QImage* image, int startY, int endY)
		: m_image(image), m_startY(startY), m_endY(endY)
	{
	}

	double linearToRgb(double linear, double gamma=2.2) {
		double rgb;

		rgb = pow(linear, 1 / gamma);

		return rgb;
	}

	void run() override
	{
		int width = m_image->width();
		for (int y = m_startY; y < m_endY; ++y)
		{
			QRgb* scanLine = reinterpret_cast<QRgb*>(m_image->scanLine(y));
			for (int x = 0; x < width; ++x)
			{
				QRgb pixel = scanLine[x];
				int red = qRed(pixel);
				int green = qGreen(pixel);
				int blue = qBlue(pixel);
				double fRed = double(red) / 255.0;
				double fGreen = double(green) / 255.0;
				double fBlue = double(blue) / 255.0;


				int newRed = qMin(255, int(linearToRgb(fRed)*255) );
				int newGreen = qMin(255, int(linearToRgb(fGreen)*255) );
				int newBlue = qMin(255, int(linearToRgb(fBlue)*255) );

				scanLine[x] = qRgb(newRed, newGreen, newBlue);
			}
		}
	}

private:
	QImage* m_image;
	int m_startY;
	int m_endY;

};
